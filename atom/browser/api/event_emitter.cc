// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event_emitter.h"

#include "atom/browser/api/event.h"
#include "native_mate/arguments.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace mate {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(mate::Arguments* args) {
  args->GetThis()->Set(StringToV8(args->isolate(), "defaultPrevented"),
                       v8::True(args->isolate()));
}

// Create a pure JavaScript Event object.
v8::Local<v8::Object> CreateEventObject(v8::Isolate* isolate) {
  if (event_template.IsEmpty()) {
    event_template.Reset(isolate, ObjectTemplateBuilder(isolate)
        .SetMethod("preventDefault", &PreventDefault)
        .Build());
  }

  return v8::Local<v8::ObjectTemplate>::New(
      isolate, event_template)->NewInstance();
}

}  // namespace

EventEmitter::EventEmitter() {
}

bool EventEmitter::CallEmit(v8::Isolate* isolate,
                            const base::StringPiece& name,
                            content::WebContents* sender,
                            IPC::Message* message,
                            ValueArray args) {
  v8::Local<v8::Object> event;
  bool use_native_event = sender && message;

  if (use_native_event) {
    mate::Handle<mate::Event> native_event = mate::Event::Create(isolate);
    native_event->SetSenderAndMessage(sender, message);
    event = v8::Local<v8::Object>::Cast(native_event.ToV8());
  } else {
    event = CreateEventObject(isolate);
  }

  // args = [name, event, args...];
  args.insert(args.begin(), event);
  args.insert(args.begin(), mate::StringToV8(isolate, name));

  // this.emit.apply(this, args);
  node::MakeCallback(isolate, GetWrapper(isolate), "emit", args.size(),
                     &args[0]);

  return event->Get(StringToV8(isolate, "defaultPrevented"))->BooleanValue();
}

}  // namespace mate
