package org.simplektble

import java.io.FileNotFoundException
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.util.Locale

private const val LIBRARY_NAME = "simplektble-jni"

/**
 * Loads the JNI library for this platform from the jar.
 *
 * The jar holds one library per architecture under native/<arch>/, named the way each operating system expects. The
 * JVM cannot load a library from inside a jar, so it is copied to a temporary directory first.
 */
internal actual fun loadNativeLibrary() {
    val os = System.getProperty("os.name").lowercase(Locale.ROOT)
    val fileName = when {
        os.contains("windows") -> "$LIBRARY_NAME.dll"
        os.contains("mac") -> "lib$LIBRARY_NAME.dylib"
        os.contains("linux") -> "lib$LIBRARY_NAME.so"
        else -> throw UnsupportedOperationException("Unsupported operating system: $os")
    }

    val arch = System.getProperty("os.arch").lowercase(Locale.ROOT)
    val archName = when {
        arch.contains("amd64") || arch.contains("x86_64") -> "x64"
        arch.contains("aarch64") || arch.contains("arm64") -> "aarch64"
        arch.contains("x86") || arch.contains("i386") || arch.contains("i686") -> "x86"
        else -> throw UnsupportedOperationException("Unsupported architecture: $arch")
    }

    val resourcePath = "/native/$archName/$fileName"
    val directory = Files.createTempDirectory("simplektble-")
    val library = directory.resolve(fileName)

    Native::class.java.getResourceAsStream(resourcePath).use { input ->
        if (input == null) throw FileNotFoundException("Native library not found: $resourcePath")
        Files.copy(input, library, StandardCopyOption.REPLACE_EXISTING)
    }

    System.load(library.toAbsolutePath().toString())

    // NOTE: Windows keeps a loaded library open, so deleting fails there and is retried at exit, in reverse order.
    directory.toFile().deleteOnExit()
    library.toFile().deleteOnExit()
    runCatching {
        Files.deleteIfExists(library)
        Files.deleteIfExists(directory)
    }
}
