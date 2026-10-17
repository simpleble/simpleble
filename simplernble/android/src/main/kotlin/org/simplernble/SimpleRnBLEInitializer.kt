package org.simplernble

import android.content.ContentProvider
import android.content.ContentValues
import android.content.Context
import android.database.Cursor
import android.net.Uri

// Runs when the app starts, before any JavaScript, to load the native library
// and give SimpleBLE the application context it hosts GATT services with.
class SimpleRnBLEInitializer : ContentProvider() {
  override fun onCreate(): Boolean {
    System.loadLibrary("SimpleRnBLE")
    setApplicationContext(context!!.applicationContext)
    return true
  }

  override fun query(uri: Uri, projection: Array<String>?, selection: String?, selectionArgs: Array<String>?, sortOrder: String?): Cursor? = null

  override fun getType(uri: Uri): String? = null

  override fun insert(uri: Uri, values: ContentValues?): Uri? = null

  override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?): Int = 0

  override fun update(uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<String>?): Int = 0

  companion object {
    @JvmStatic private external fun setApplicationContext(context: Context)
  }
}
