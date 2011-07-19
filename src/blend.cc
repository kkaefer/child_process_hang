#include "blend.h"
#include "reader.h"

using namespace v8;
using namespace node;

Handle<Value> Blend(const Arguments& args) {
    HandleScope scope;

    Local<Function> callback;
    Local<Object> options;

    if (args.Length() == 0 || !args[0]->IsArray()) {
        return TYPE_EXCEPTION("First argument must be an array of Buffers.");
    } else if (args.Length() == 1) {
        return TYPE_EXCEPTION("Second argument must be a function");
    } else if (args.Length() == 2) {
        // No options provided.
        if (!args[1]->IsFunction()) {
            return TYPE_EXCEPTION("Second argument must be a function.");
        }
        callback = Local<Function>::Cast(args[1]);
    } else if (args.Length() >= 3) {
        if (!args[1]->IsObject()) {
            return TYPE_EXCEPTION("Second argument must be a an options object.");
        }
        options = Local<Object>::Cast(args[1]);

        if (!args[2]->IsFunction()) {
            return TYPE_EXCEPTION("Third argument must be a function.");
        }
        callback = Local<Function>::Cast(args[2]);
    }


    BlendFormat format = BLEND_FORMAT_PNG;

    Local<Array> buffers = Local<Array>::Cast(args[0]);
    uint32_t length = buffers->Length();
    if (length < 1) {
        return TYPE_EXCEPTION("First argument must contain at least one Buffer.");
    } else if (length == 1) {
        if (!Buffer::HasInstance(buffers->Get(0))) {
            return TYPE_EXCEPTION("All elements must be Buffers.");
        } else {
            // Directly pass through buffer if it's the only one.
            Local<Value> argv[] = {
                Local<Value>::New(Null()),
                Local<Value>::New(buffers->Get(0))
            };
            TRY_CATCH_CALL(Context::GetCurrent()->Global(), callback, 2, argv);
        }
    } else {
        BlendBaton* baton = new BlendBaton(callback, format, 90);
        for (uint32_t i = 0; i < length; i++) {
            if (!Buffer::HasInstance(buffers->Get(i))) {
                delete baton;
                return TYPE_EXCEPTION("All elements must be Buffers.");
            } else {
                baton->add(buffers->Get(i)->ToObject());
            }
        }

        eio_custom(EIO_Blend, EIO_PRI_DEFAULT, EIO_AfterBlend, baton);
    }

    return scope.Close(Undefined());
}

int EIO_Blend(eio_req *req) {
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);

    int total = baton->buffers.size();
    int size = 0;
    unsigned int* images[total];
    for (int i = 0; i < total; i++) images[i] = NULL;

    unsigned long width = 0;
    unsigned long height = 0;
    bool alpha = true;

    // Iterate from the last to first image.
    ImageBuffers::reverse_iterator image = baton->buffers.rbegin();
    ImageBuffers::reverse_iterator end = baton->buffers.rend();
    for (; image < end; image++) {
        ImageReader* layer = ImageReader::create((*image).first, (*image).second);

        // Skip invalid images.
        if (layer == NULL || layer->width == 0 || layer->height == 0) {
            baton->error = true;
            baton->message = layer->message;
            delete layer;
            break;
        }

        if (size == 0) {
            width = layer->width;
            height = layer->height;
            if (!layer->alpha) {
                baton->result = (*image).first;
                baton->length = (*image).second;
                delete layer;
                break;
            }
        } else if (layer->width != width || layer->height != height) {
            baton->error = true;
            baton->message = "Image dimensions don't match";
            delete layer;
            break;
        }

        images[size] = (unsigned int*)layer->decode();
        if (images[size] == NULL) {
            // Decoding failed.
            baton->error = true;
            baton->message = layer->message;
            delete layer;
            break;
        }

        size++;

        if (!layer->alpha) {
            // Skip decoding more layers.
            alpha = false;
            delete layer;
            break;
        }

        delete layer;
    }

    for (int i = 0; i < size; i++) {
        if (images[i] != NULL) {
            free(images[i]);
            images[i] = NULL;
        }
    }

    return 0;
}

int EIO_AfterBlend(eio_req *req) {
    HandleScope scope;
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);

    if (!baton->error && baton->result) {
        Local<Value> argv[] = {
            Local<Value>::New(Null()),
            Local<Value>::New(Buffer::New((char*)baton->result, baton->length)->handle_)
        };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 2, argv);
    } else {
        Local<Value> argv[] = {
            Local<Value>::New(Exception::Error(String::New(baton->message.c_str())))
        };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 1, argv);
    }

    delete baton;
    return 0;
}

extern "C" void init (Handle<Object> target) {
    NODE_SET_METHOD(target, "blend", Blend);

    target->Set(
        String::NewSymbol("libpng"),
        String::NewSymbol(PNG_LIBPNG_VER_STRING),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete)
    );
}
