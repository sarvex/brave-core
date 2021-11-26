/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.KeyboardVisibilityDelegate;

/**
 * The preference used to reset Brave Wallet.
 */
public class BraveWalletResetPreference
        extends Preference implements Preference.OnPreferenceClickListener, ConnectionErrorHandler {
    private String TAG = "BraveWalletResetPreference";

    private int mPrefAccentColor;
    private final String mConfirmationPhrase;
    private KeyringController mKeyringController;

    /**
     * Constructor for BraveWalletResetPreference.
     */
    public BraveWalletResetPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        Resources resources = getContext().getResources();
        mPrefAccentColor =
                ApiCompatibilityUtils.getColor(resources, R.color.wallet_error_text_color);
        mConfirmationPhrase =
                resources.getString(R.string.brave_wallet_reset_settings_confirmation_phrase);
        setOnPreferenceClickListener(this);

        InitKeyringController();
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        TextView titleView = (TextView) holder.findViewById(android.R.id.title);
        titleView.setTextColor(mPrefAccentColor);
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        showBraveWalletResetDialog();
        return true;
    }

    @Override
    public void onDetached() {
        super.onDetached();
        if (mKeyringController != null) {
            mKeyringController.close();
        }
    }

    private void showBraveWalletResetDialog() {
        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.brave_wallet_reset_content, null);
        final TextView textView = (TextView) view.findViewById(R.id.brave_wallet_reset_textview);
        final EditText input = (EditText) view.findViewById(R.id.brave_wallet_reset_edittext);

        textView.setText(getContext().getString(
                R.string.brave_wallet_reset_settings_confirmation, mConfirmationPhrase));

        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button == AlertDialog.BUTTON_POSITIVE) {
                    if (mKeyringController != null) {
                        String inputText = input.getText().toString().trim();
                        if (TextUtils.equals(inputText, mConfirmationPhrase)) {
                            Log.w(TAG, "Reset");
                            mKeyringController.reset();
                            Utils.setCryptoOnboarding(true);
                        }
                        mKeyringController.close();
                    } else {
                        Log.w(TAG, "mKeyringController is null");
                    }
                } else {
                    dialog.dismiss();
                }
            }
        };

        AlertDialog.Builder alert =
                new AlertDialog.Builder(getContext(), R.style.Theme_Chromium_AlertDialog);
        AlertDialog alertDialog =
                alert.setTitle(R.string.brave_wallet_reset_settings_option)
                        .setView(view)
                        .setPositiveButton(R.string.brave_wallet_confirm_text, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                KeyboardVisibilityDelegate.getInstance().showKeyboard(input);
            }
        });
        alertDialog.show();
        final Button okButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        okButton.setEnabled(false);

        input.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {}

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // Disable ok button if input is invalid
                String inputText = s.toString().trim();

                okButton.setEnabled(TextUtils.equals(inputText, mConfirmationPhrase));
            }
        });
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringController.close();
        mKeyringController = null;
        InitKeyringController();
    }

    private void InitKeyringController() {
        if (mKeyringController != null) {
            return;
        }

        mKeyringController = KeyringControllerFactory.getInstance().getKeyringController(this);
    }
}