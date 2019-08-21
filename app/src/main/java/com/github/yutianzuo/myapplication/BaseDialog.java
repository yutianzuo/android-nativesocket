package com.github.yutianzuo.myapplication;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.LayoutRes;
import android.support.v7.app.AlertDialog;
import android.util.DisplayMetrics;
import android.util.SparseArray;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

public class BaseDialog extends AlertDialog {

    public static final int LEFT_VIEW = R.id.base_dlg_left;
    public static final int RIGHT_VIEW = R.id.base_dlg_right;
    public static final int MESSAGE_VIEW = R.id.base_dlg_title;
    private static final int NOT_SET_VALUE = 0;
    private static final float DEFAULT_WIDTH_PERCENT = 0.7f;
    private static final float DEFAULT_HEIGHT_PERCENT = 0.4f;
    private SparseArray<View> mViewCache = new SparseArray<>();
    private View mContentView;
    private Builder mBuilder;
    private static int mBaseLayout = R.layout.base_dlg_layout;

    private BaseDialog(Builder builder, int resStyle) {
        super(builder.context, resStyle);
        mBuilder = builder;
        mContentView = View.inflate(builder.context, builder.layout, null);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(mContentView);
        setCanceledOnTouchOutside(mBuilder.canceledOnTouchOutside);
        setCancelable(mBuilder.cancelForced);

        Window win = getWindow();
        if (win == null) return;
        if (mBuilder.resAnim != -1)
            win.setWindowAnimations(mBuilder.resAnim);

        WindowManager.LayoutParams lp = win.getAttributes();
        lp.gravity = mBuilder.gravity;
        lp.height = mBuilder.height;
        lp.width = mBuilder.width;
        win.setAttributes(lp);
        setTextAndListeners();
        mBuilder.release();
        mBuilder = null;
    }

    /**
     * 配置BaseDialog的基础样式,包含标题、确定和取消按钮的Dialog布局
     *
     * @param layout layout的xml里必须包含base_dlg_title,base_dlg_left,base_dlg_right三个ID
     */
    public static void config(@LayoutRes int layout) {
        mBaseLayout = layout;
    }

    /**
     * 重新设置对话框文案
     */
    public void setText(@IdRes int viewRes, CharSequence sequence) {
        TextView textView = getView(viewRes);
        if (textView == null) return;
        textView.setText(sequence);
    }

    public View getRootView() {
        return mContentView;
    }

    /**
     * 获取view
     */
    @SuppressWarnings("unchecked")
    public <T extends View> T getView(@IdRes int viewId) {
        View child = mViewCache.get(viewId);
        if (child == null) {
            child = mContentView.findViewById(viewId);
            if (child == null) return null;
            mViewCache.put(viewId, child);
        }
        return (T) child;
    }

    @Override
    public void show() {
        try {
            super.show();
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    @Override
    public void dismiss() {
        try {
            super.dismiss();
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    private void setTextAndListeners() {
        int size = mBuilder.texts.size();
        for (int i = 0; i < size; i++)
            setText(mBuilder.texts.keyAt(i), mBuilder.texts.valueAt(i));
        size = mBuilder.listeners.size();
        for (int i = 0; i < size; i++) {
            getView(mBuilder.listeners.keyAt(i)).setOnClickListener(mBuilder.listeners.valueAt(i));
        }

        setOptListener(LEFT_VIEW);
        setOptListener(RIGHT_VIEW);
    }

    private void setOptListener(int id) {
        View view = getView(id);
        if (view != null) {
            final View.OnClickListener listener = mBuilder.listeners.get(id);
            view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    dismiss();
                    if (listener != null)
                        listener.onClick(v);
                }
            });
        }
    }

    public static Builder newBuilder(Context context) {
        return new Builder(context);
    }

    /**
     * 建立Builder模式
     */
    public static class Builder {
        /**
         * 上下文
         */
        private Context context;
        /**
         * 高
         */
        private int height = -1;
        private float heightPercent = 0;
        /**
         * 宽
         */
        private int width = -1;
        private float widthPercent = 0;

        /**
         * 是否点击dialog外面背景取消dialog
         */
        private boolean canceledOnTouchOutside = true;

        private boolean cancelForced = true;
        /**
         * dialog style
         */
        private int resStyle = -1;
        /**
         * 进出动画
         */
        private int resAnim = -1;
        /**
         * 显示位置 默认居中
         */
        private int gravity = Gravity.CENTER;

        private SparseArray<View.OnClickListener> listeners;
        private SparseArray<String> texts;
        private int layout;

        public Builder(Context context) {
            this.context = context;
            layout = NOT_SET_VALUE;
            resStyle = R.style.BaseDialog;
            widthPercent = DEFAULT_WIDTH_PERCENT;
            heightPercent = DEFAULT_HEIGHT_PERCENT;
            height = ViewGroup.LayoutParams.WRAP_CONTENT;
            width = NOT_SET_VALUE;
            listeners = new SparseArray<>();
            texts = new SparseArray<>();
        }

        /**
         * 自定义布局
         */
        public Builder layout(@LayoutRes int resView) {
            layout = resView;
            return this;
        }

        /**
         * 设置对话框高度
         */
        public Builder height(int val) {
            height = val;
            return this;
        }

        /**
         * 设置对话框宽度
         */
        public Builder width(int val) {
            width = val;
            return this;
        }

        /**
         * 设置对话框占屏百分比高度
         */
        public Builder heightPercent(float heightPercent) {
            this.heightPercent = heightPercent;
            return this;
        }

        /**
         * 设置对话框占屏百分比宽度
         */
        public Builder widthPercent(float widthPercent) {
            this.widthPercent = widthPercent;
            return this;
        }

        /**
         * 设置对话框style
         */
        public Builder style(int resStyle) {
            this.resStyle = resStyle;
            return this;
        }

        /**
         * 设置对话框显示于关闭动画
         */
        public Builder animation(int resAnim) {
            this.resAnim = resAnim;
            return this;
        }

        /**
         * 设置对话框位置
         */
        public Builder gravity(int gravity) {
            this.gravity = gravity;
            return this;
        }

        /**
         * 设置对话框是否能取消
         */
        public Builder canceledOnTouchOutside(boolean cancelable) {
            canceledOnTouchOutside = cancelable;
            return this;
        }

        public Builder cancelable(boolean cancelable) {
            cancelForced = cancelable;
            return this;
        }

        /**
         * 设置对话框点击事件
         */
        public Builder setListener(int viewRes, View.OnClickListener listener) {
            listeners.put(viewRes, listener);
            return this;
        }

        /**
         * 设置对话框文案
         */
        public Builder setText(int viewRes, CharSequence sequence) {
            texts.put(viewRes, sequence.toString());
            return this;
        }

        public Builder setText(int viewRes, CharSequence sequence, View.OnClickListener listener) {
            texts.put(viewRes, sequence.toString());
            if (listener != null)
                listeners.put(viewRes, listener);
            return this;
        }


        public BaseDialog build() {
            DisplayMetrics dm = context.getResources().getDisplayMetrics();
            if (width == NOT_SET_VALUE)
                width = (int) (dm.widthPixels * widthPercent);
            if (height == NOT_SET_VALUE)
                height = (int) (dm.heightPixels * heightPercent);
            if (layout == NOT_SET_VALUE)
                layout = mBaseLayout;
            return new BaseDialog(this, resStyle);
        }

        public void show() {
            build().show();
        }

        public void release() {
            listeners.clear();
            texts.clear();
            listeners = null;
            texts = null;
        }
    }
}