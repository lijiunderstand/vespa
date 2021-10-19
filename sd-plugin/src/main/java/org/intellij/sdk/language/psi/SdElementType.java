package org.intellij.sdk.language.psi;

import com.intellij.psi.tree.IElementType;
import org.intellij.sdk.language.SdLanguage;
import org.jetbrains.annotations.NonNls;
import org.jetbrains.annotations.NotNull;

public class SdElementType extends IElementType {
    
    public SdElementType(@NotNull @NonNls String debugName) {
        super(debugName, SdLanguage.INSTANCE);
    }
}