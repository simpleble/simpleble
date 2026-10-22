package org.simplejavable;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * Runs the blocking calls behind the {@code Async} methods.
 * <p>
 * ForkJoinPool.commonPool() is sized for computation, so blocking BLE calls there would stall the application's
 * other asynchronous work. The threads are daemons, so they don't keep the JVM from exiting.
 */
final class AsyncExecutor {
    static final Executor INSTANCE = Executors.newCachedThreadPool(task -> {
        Thread thread = new Thread(task, "SimpleJavaBLE");
        thread.setDaemon(true);
        return thread;
    });

    private AsyncExecutor() {}
}
