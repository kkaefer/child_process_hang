#include <v8.h>
#include <node.h>
#include <setjmp.h>

using namespace v8;
using namespace node;

jmp_buf buffer;

struct Baton { Persistent<Function> callback; };

static int EIO_Jump(eio_req *req) {
    if (!setjmp(buffer)) {
        longjmp(buffer, 1);
    }

    return 0;
}

static int EIO_AfterJump(eio_req *req) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);
    baton->callback->Call(Context::GetCurrent()->Global(), 0, NULL);
    return 0;
}

static Handle<Value> Jump(const Arguments& args) {
    HandleScope scope;
    Baton* baton = new Baton();
    baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));
    eio_custom(EIO_Jump, EIO_PRI_DEFAULT, EIO_AfterJump, baton);
    return scope.Close(Undefined());
}

extern "C" void init (Handle<Object> target) {
    NODE_SET_METHOD(target, "jump", Jump);
}
