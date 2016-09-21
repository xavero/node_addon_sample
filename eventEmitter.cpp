#ifndef _WIN32
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
#include <nan.h>

using namespace Nan;

class SleepWorker : public AsyncWorker {
 public:
  SleepWorker(Callback *callback, int milliseconds)
    : AsyncWorker(callback), milliseconds(milliseconds) {}
  ~SleepWorker() {}

  void Execute () {
	// this executes outside node main thread. 
	// Cannot call any v8 or nan objects and functions here,
	// but we can run long C functions that would otherwise hold our main Thread
    Sleep(milliseconds);
  }
  
  // its not necessay to implement this function, 
  // unless you want a diferent callback function to run,
  // or pass some response to js
  //virtual void HandleOKCallback()
  //{
  //callback->Call(0, NULL);
  //}

 private:
  int milliseconds;
};

class MyObject : public ObjectWrap {
 public:
  static NAN_MODULE_INIT(Init);

 private:
  MyObject();
  ~MyObject();

  static NAN_METHOD(New);
  static NAN_METHOD(CallEmit);
  static NAN_METHOD(AsyncCallback);
  static NAN_METHOD(StartLoop);
  static Persistent<v8::Function> constructor;
};

struct async_event_data 
{
    uv_work_t request;
    std::string *event_name;
	std::string *someArg;
};

Persistent<v8::Function> MyObject::constructor;

MyObject::MyObject() {
}

MyObject::~MyObject() {
}

NAN_MODULE_INIT(MyObject::Init) {
  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New<v8::String>("MyObject").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "call_emit", CallEmit);
  SetPrototypeMethod(tpl, "callbackSample", AsyncCallback);
  SetPrototypeMethod(tpl, "startLoop", StartLoop);

  constructor.Reset(tpl->GetFunction());
  Set(target, Nan::New("MyObject").ToLocalChecked(), tpl->GetFunction());
}

NAN_METHOD(MyObject::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowError("`new` required");
  }
  
    MyObject* obj = new MyObject();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(MyObject::CallEmit) {
  // this just shows how C function can call a js function
  v8::Local<v8::Value> argv[2] = {
    Nan::New("ping").ToLocalChecked(),  // event name
	Nan::New("pong").ToLocalChecked()
  };

  MakeCallback(info.This(), "emit", 2, argv);
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(MyObject::AsyncCallback) {
  // this function shows how we can do async work, outside the main thread, 
  // and later raise a callback js function, that was passed as argument.
  Callback *callback = new Callback(info[1].As<v8::Function>());
  AsyncQueueWorker(new SleepWorker(callback, To<uint32_t>(info[0]).FromJust()));
}

MyObject* myObjectThreadLoop;
uv_thread_t mythread;
int threadnum = 1;

NAN_INLINE void noop_execute (uv_work_t* req) {
	// dont remember why this exists
}

NAN_INLINE void callback_async_event (uv_work_t* req) {
  Nan::HandleScope scope;
  
  // here we are executing in the node thread, so we can call nan and v8 functions
  
  // we could retrieve some data that was passed around
  async_event_data* data = static_cast<async_event_data*>(req->data);
  
  // arguments to sendo to js function
  v8::Local<v8::Value> emit_argv[] = { Nan::New("someEvent").ToLocalChecked(), Nan::New("someArg").ToLocalChecked() };
  
  // calling emit function on our object
  MakeCallback(myObjectThreadLoop->handle(), "emit", 2, emit_argv);

  delete data;
}

void emit_event() {
	// we could use this struct to send some data from C to JS Land,
	// but for now i will just let this empty
	async_event_data *asyncdata = new async_event_data();
	asyncdata->request.data = (void *) asyncdata;
	
	// here we schedule a function to be called on the main/node thread
	uv_queue_work(uv_default_loop(), &asyncdata->request, noop_execute, reinterpret_cast<uv_after_work_cb>(callback_async_event));
}

void dumbthreadloop(void *n) {
	while(true) {
		// infinite loop where we just sleep for 2 seconds
		Sleep(2000);
		// then we wake, send a message to js land
		emit_event();
	}
}

NAN_METHOD(MyObject::StartLoop) {
	
	if(myObjectThreadLoop) return ThrowError("cant call twice"); 
	
	myObjectThreadLoop = Nan::ObjectWrap::Unwrap<MyObject>(info.Holder());
	
	// i dont really know C++, could be a better way to do Threads.
	// we will use this thread as a example how a thread could
	// call JS functions. Useful when you are writing a driver/interface 
	// with hardware, messaging/queue servers, etc.
	uv_thread_create(&mythread, dumbthreadloop, &threadnum);
}

NODE_MODULE(makecallback, MyObject::Init)