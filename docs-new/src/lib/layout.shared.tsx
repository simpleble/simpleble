import type { BaseLayoutProps } from "fumadocs-ui/layouts/shared";
import type { DocsLayoutProps } from "fumadocs-ui/layouts/docs";

import fs from "node:fs";
import path from "node:path";
import { LogoWithTheme } from "@/components/logo-with-theme";

const version = fs
  .readFileSync(path.resolve(process.cwd(), "../VERSION"), "utf-8")
  .trim();

export function baseOptions(): BaseLayoutProps {
  return {
    nav: {
      title: (
        <LogoWithTheme
          width={152}
          height={50}
          className="h-8 md:h-9 w-auto"
          priority
        />
      ),
    },
  };
}

export function docsSidebarOptions(): DocsLayoutProps["sidebar"] {
  return {
    footer: (
      <div className="flex flex-col gap-2 p-2">
        <div className="flex items-center justify-between">
          <span className="text-[10px] uppercase tracking-widest font-semibold text-fd-muted-foreground/70">
            Version
          </span>
          <span className="text-[11px] font-mono font-medium text-fd-primary bg-fd-primary/5 px-1.5 py-0.5 rounded border border-fd-primary/10">
            {version}
          </span>
        </div>
      </div>
    ),
  };
}
