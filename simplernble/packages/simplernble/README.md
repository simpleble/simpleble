# simplernble

React Native Bluetooth Low Energy library using SimpleBLE with Nitro Modules.

## iOS Build Issue Fix

If you encounter this error when building your iOS project:

```
Property with 'retain (or strong)' attribute must be of object type
/Users/.../Pods/Headers/Public/React-Core/React/RCTBridgeModule.h:164:1
```

Add this to your `Podfile` in the `post_install` block:

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

Then run `pod install` again.

## Installation

```bash
npm install simplernble
```

## Usage

```typescript
import { Adapter } from 'simplernble';

// Your code here
```
