# JNI resolves these types and callback method names dynamically.
-keep class org.simpleble.android.SimpleDroidBleException {
    public <init>(java.lang.String);
}
-keep class org.simpleble.android.Service {
    public <init>(java.lang.String, java.util.List);
}
-keep class org.simpleble.android.Characteristic {
    public <init>(java.lang.String, java.util.List, boolean, boolean, boolean, boolean, boolean);
}
-keep class org.simpleble.android.Descriptor {
    public <init>(java.lang.String);
}
-keep interface org.simpleble.android.Adapter$Callback { *; }
-keep interface org.simpleble.android.Peripheral$Callback { *; }
-keep interface org.simpleble.android.Peripheral$DataCallback { *; }
-keep interface org.simpleble.android.LocalPeripheral$Callback { *; }
-keep interface org.simpleble.android.LocalCharacteristic$Callback { *; }
-keep class * implements org.simpleble.android.Adapter$Callback { *; }
-keep class * implements org.simpleble.android.Peripheral$Callback { *; }
-keep class * implements org.simpleble.android.Peripheral$DataCallback { *; }
-keep class * implements org.simpleble.android.LocalPeripheral$Callback { *; }
-keep class * implements org.simpleble.android.LocalCharacteristic$Callback { *; }
-keepclasseswithmembernames,includedescriptorclasses class org.simpleble.android.** {
    native <methods>;
}
