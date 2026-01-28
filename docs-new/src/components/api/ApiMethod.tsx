"use client";

import { useState } from "react";
import { cn } from "@/lib/cn";
import { Code2 } from "lucide-react";
import { DynamicCodeBlock } from "fumadocs-ui/components/dynamic-codeblock";
import ReactMarkdown from "react-markdown";

interface ApiMethodProps {
  signature: string;
  brief?: string;
  detailed?: string;
  parameters?: Array<{
    name: string;
    type: string;
    description?: string;
  }>;
}

/**
 * Inline code with syntax highlighting using Fumadocs DynamicCodeBlock.
 */
function InlineCode({ code, className }: { code: string; className?: string }) {
  return (
    <div
      className={cn(
        "min-w-0 flex-1 [&_pre]:!m-0 [&_pre]:!p-0 [&_pre]:!bg-transparent [&_code]:!text-sm [&_code]:!font-mono [&_pre]:!whitespace-pre-wrap [&_pre]:!break-words [&_code]:!whitespace-pre-wrap [&_code]:!break-words [&_pre]:!overflow-visible [&_code]:!overflow-visible",
        className,
      )}
    >
      <DynamicCodeBlock
        lang="cpp"
        code={code}
        codeblock={{
          allowCopy: false,
        }}
      />
    </div>
  );
}

export function ApiMethod({
  signature,
  brief,
  detailed,
  parameters,
}: ApiMethodProps) {
  const [isParamsOpen, setIsParamsOpen] = useState(false);
  const [isDetailsOpen, setIsDetailsOpen] = useState(false);

  return (
    <div
      className={cn(
        "my-4 rounded-lg border border-fd-border/40 bg-fd-card/30 transition-all hover:border-fd-border/70 hover:shadow-md hover:bg-fd-card/50",
      )}
    >
      {/* Method Signature */}
      <div
        className={cn(
          "w-full flex items-center gap-3 bg-gradient-to-r from-fd-muted/30 to-transparent px-4 py-3 text-left transition-colors",
        )}
      >
        <div className="min-w-0 flex-1 py-0.5">
          <InlineCode code={signature} />
        </div>
      </div>

      {/* Method Details */}
      {(brief || detailed || (parameters && parameters.length > 0)) && (
        <div className="px-4 pb-4 space-y-4">
          {/* Brief and Detailed descriptions - Collapsible */}
          {(brief || detailed) && (
            <div className="space-y-4">
              <button
                onClick={() => setIsDetailsOpen(!isDetailsOpen)}
                className={cn(
                  "flex items-center gap-2 px-3 py-1.5 text-xs font-medium transition-all cursor-pointer group rounded-md border",
                  "text-fd-muted-foreground border-fd-border/50 ",
                )}
              >
                <div className="relative w-3 h-3 flex items-center justify-center">
                  <span
                    className={cn(
                      "absolute w-full h-0.5 bg-current rounded-full transition-all duration-500 ease-in-out",
                      isDetailsOpen ? "rotate-180" : "rotate-0",
                    )}
                  />
                  <span
                    className={cn(
                      "absolute w-full h-0.5 bg-current rounded-full transition-all duration-500 ease-in-out",
                      isDetailsOpen ? "rotate-180" : "rotate-90",
                    )}
                  />
                </div>
                {isDetailsOpen ? "Hide details" : "Show details"}
              </button>

              {isDetailsOpen && (
                <div className="space-y-3 animate-in fade-in slide-in-from-top-1 duration-500 ease-in-out">
                  {brief && (
                    <div className="text-sm text-fd-muted-foreground leading-relaxed prose prose-sm max-w-none">
                      <ReactMarkdown
                        components={{
                          a: ({ href, children }) => (
                            <a
                              href={href}
                              className="text-fd-primary hover:underline font-medium"
                            >
                              {children}
                            </a>
                          ),
                          p: ({ children }) => <>{children}</>,
                        }}
                      >
                        {brief}
                      </ReactMarkdown>
                    </div>
                  )}

                  {detailed && (
                    <div className="text-sm text-fd-muted-foreground/90 prose prose-sm max-w-none">
                      <ReactMarkdown
                        components={{
                          a: ({ href, children }) => (
                            <a
                              href={href}
                              className="text-fd-primary hover:underline font-medium"
                            >
                              {children}
                            </a>
                          ),
                          p: ({ children }) => (
                            <p className="leading-relaxed mb-2 last:mb-0">
                              {children}
                            </p>
                          ),
                        }}
                      >
                        {detailed}
                      </ReactMarkdown>
                    </div>
                  )}
                </div>
              )}
            </div>
          )}

          {/* Parameters - Collapsible */}
          {parameters && parameters.length > 0 && (
            <div className="space-y-4">
              <button
                onClick={() => setIsParamsOpen(!isParamsOpen)}
                className={cn(
                  "flex items-center gap-2 px-3 py-1.5 text-xs font-medium transition-all cursor-pointer group rounded-md border",
                  "text-fd-muted-foreground border-fd-border/50 ",
                )}
              >
                <div className="relative w-3 h-3 flex items-center justify-center">
                  <span
                    className={cn(
                      "absolute w-full h-0.5 bg-current rounded-full transition-all duration-500 ease-in-out",
                      isParamsOpen ? "rotate-180" : "rotate-0",
                    )}
                  />
                  <span
                    className={cn(
                      "absolute w-full h-0.5 bg-current rounded-full transition-all duration-500 ease-in-out",
                      isParamsOpen ? "rotate-180" : "rotate-90",
                    )}
                  />
                </div>
                {isParamsOpen ? "Hide parameters" : "Show parameters"}
              </button>

              {isParamsOpen && (
                <div className="animate-in fade-in slide-in-from-top-1 duration-700 ease-in-out">
                  <h5 className="text-xs font-semibold text-fd-muted-foreground uppercase tracking-wider mb-3">
                    Parameters
                  </h5>
                  <div className="space-y-2.5">
                    {parameters.map((param, i) => (
                      <div
                        key={i}
                        className="flex items-center gap-2 pl-3 border-l-2 border-fd-primary/20"
                      >
                        <code className="text-xs font-mono text-fd-primary bg-fd-primary/5 px-2 py-1 rounded border border-fd-primary/10">
                          {param.name}
                        </code>
                        <div className="text-xs text-fd-muted-foreground bg-fd-muted/30 px-2 py-1 rounded break-all flex-1">
                          <InlineCode
                            code={param.type}
                            className="[&_code]:!text-xs"
                          />
                        </div>
                        {param.description && (
                          <span className="text-xs text-fd-muted-foreground/80 italic">
                            {param.description}
                          </span>
                        )}
                      </div>
                    ))}
                  </div>
                </div>
              )}
            </div>
          )}
        </div>
      )}
    </div>
  );
}
