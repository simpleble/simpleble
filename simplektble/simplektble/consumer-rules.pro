# The JNI library looks these types and callback methods up by name.
-keep class org.simplektble.SimpleKtBleException {
    <init>(int, java.lang.String);
}
-keep class org.simplektble.Service {
    <init>(java.lang.String, byte[], java.util.List);
}
-keep class org.simplektble.Characteristic {
    <init>(java.lang.String, java.util.List, boolean, boolean, boolean, boolean, boolean);
}
-keep class org.simplektble.Descriptor {
    <init>(java.lang.String);
}
-keep interface org.simplektble.AdapterCallback { *; }
-keep interface org.simplektble.PeripheralCallback { *; }
-keep interface org.simplektble.DataCallback { *; }
-keep interface org.simplektble.LocalPeripheralCallback { *; }
-keep interface org.simplektble.LocalCharacteristicCallback { *; }
-keep class * implements org.simplektble.AdapterCallback { *; }
-keep class * implements org.simplektble.PeripheralCallback { *; }
-keep class * implements org.simplektble.DataCallback { *; }
-keep class * implements org.simplektble.LocalPeripheralCallback { *; }
-keep class * implements org.simplektble.LocalCharacteristicCallback { *; }
-keepclasseswithmembernames,includedescriptorclasses class org.simplektble.** {
    native <methods>;
}
