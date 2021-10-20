package org.intellij.sdk.language;

import com.intellij.psi.codeStyle.CodeStyleSettings;
import com.intellij.psi.codeStyle.CustomCodeStyleSettings;

/**
 * This class represent a code style settings, and creates an option page in settings/preferences.
 * @author shahariel
 */
public class SdCodeStyleSettings extends CustomCodeStyleSettings {
    
    public SdCodeStyleSettings(CodeStyleSettings settings) {
        super("SdCodeStyleSettings", settings);
    }
    
}
