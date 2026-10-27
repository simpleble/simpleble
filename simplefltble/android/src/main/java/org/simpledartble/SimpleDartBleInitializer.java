package org.simpledartble;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

// Runs when the app starts, before any Dart code, to load the native library
// and give SimpleBLE the application context it hosts GATT services with.
public final class SimpleDartBleInitializer extends ContentProvider {
    @Override
    public boolean onCreate() {
        System.loadLibrary("simpledartble");
        setContext(getContext().getApplicationContext());
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        return null;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        return null;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        return 0;
    }

    private static native void setContext(Context context);
}
