package com.github.yutianzuo.myapplication;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.util.Log;

import java.io.File;

public class FileProviderUtils {

    public static Uri getUriForFile(Context mContext, File file) {
        Uri fileUri = null;
        if (Build.VERSION.SDK_INT >= 24) {
            fileUri = getUriForFile24(mContext, file);
        } else {
            fileUri = Uri.fromFile(file);
        }
        return fileUri;
    }

    public static Uri getUriForFile24(Context mContext, File file) {
        Uri fileUri = null;
        try {
            fileUri = androidx.core.content.FileProvider.getUriForFile(mContext,
                    BuildConfig.APPLICATION_ID + ".provider",
                    file);
        } catch (Throwable e) {
            Log.e("FileProviderUtils", e.toString());
        }
        return fileUri;
    }

    public static void setIntentDataAndType(Context mContext,
            Intent intent,
            String type,
            File file,
            boolean writeAble) {
        if (Build.VERSION.SDK_INT >= 24) {
            intent.setDataAndType(getUriForFile(mContext, file), type);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (writeAble) {
                intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            }
        } else {
            intent.setDataAndType(Uri.fromFile(file), type);
        }
    }
}