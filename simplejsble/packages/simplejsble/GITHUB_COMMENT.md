# GitHub Issue Comment for #29681

Copy and paste this comment on: https://github.com/facebook/react-native/issues/29681

---

Hey! We ran into this same issue while building our React Native library (`simplejsble`). 

The problem is in `RCTBridgeModule.h` where there's this line:

```objc
@property (nonatomic, strong, readonly) dispatch_queue_t methodQueue RCT_DEPRECATED;
```

The compiler complains because `dispatch_queue_t` isn't an Objective-C object - it's a C type (basically a pointer to a struct). The `strong` keyword only works with Objective-C objects, so when the compiler sees this, it throws an error.

I know the property is deprecated, but that doesn't help because deprecated code still gets compiled. The deprecation just means "don't use this" but the header file still needs to compile.

What's weird is that not everyone sees this error. I'm curious - has anyone figured out what triggers it? Is it related to deployment targets, Xcode settings, CocoaPods versions, or something else? It seems pretty inconsistent.

Anyway, we found a workaround that's been working for our users. You can add this to your `Podfile` in the `post_install` block:

```ruby
post_install do |installer|
  # Fix RCTBridgeModule.h dispatch_queue_t property issue
  # See: https://github.com/facebook/react-native/issues/29681
  rct_bridge_module_h = File.join(installer.sandbox.root, 'Headers/Public/React-Core/React/RCTBridgeModule.h')
  
  if File.exist?(rct_bridge_module_h)
    text = File.read(rct_bridge_module_h)
    new_contents = text.gsub(
      /@property \(nonatomic, strong, readonly\) dispatch_queue_t methodQueue RCT_DEPRECATED;/,
      '@property (nonatomic, assign, readonly) dispatch_queue_t methodQueue RCT_DEPRECATED;'
    )
    
    if text != new_contents
      File.open(rct_bridge_module_h, "w") { |file| file.puts new_contents }
      puts "✓ Patched RCTBridgeModule.h methodQueue property (strong -> assign)"
    end
  end
end
```

This just changes `strong` to `assign`, which works fine for non-object types. It's safe because GCD queues handle their own memory management, so we don't need ARC to manage them.

We've tested this with React Native 0.81.5 and it works. The fix from PR #19479 would be the proper solution upstream, but until that gets merged, this workaround should help anyone else hitting this issue.
