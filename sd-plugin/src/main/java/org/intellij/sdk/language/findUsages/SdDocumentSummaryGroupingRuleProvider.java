package org.intellij.sdk.language.findUsages;

import com.intellij.openapi.project.Project;
import com.intellij.usages.impl.FileStructureGroupRuleProvider;
import com.intellij.usages.rules.UsageGroupingRule;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/**
 * This class is used for the extension (in plugin.xml) to the class SdDocumentSummaryGroupingRule.
 * @author shahariel
 */
public class SdDocumentSummaryGroupingRuleProvider implements FileStructureGroupRuleProvider {
    
    @Override
    public @Nullable UsageGroupingRule getUsageGroupingRule(@NotNull Project project) {
        return new SdDocumentSummaryGroupingRule();
    }
}
