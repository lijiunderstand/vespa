package org.intellij.sdk.language;

import com.intellij.openapi.fileTypes.SyntaxHighlighter;
import com.intellij.openapi.fileTypes.SyntaxHighlighterFactory;
import com.intellij.openapi.project.Project;
import com.intellij.openapi.vfs.VirtualFile;
import org.jetbrains.annotations.NotNull;

/**
 * This class is used for the extension (in plugin.xml) to the class SdSyntaxHighlighter.
 * @author shahariel
 */
public class SdSyntaxHighlighterFactory extends SyntaxHighlighterFactory {
    
    @NotNull
    @Override
    public SyntaxHighlighter getSyntaxHighlighter(Project project, VirtualFile virtualFile) {
        return new SdSyntaxHighlighter();
    }
    
}
