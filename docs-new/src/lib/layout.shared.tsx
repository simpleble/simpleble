import type { BaseLayoutProps } from "fumadocs-ui/layouts/shared";
import type { DocsLayoutProps } from "fumadocs-ui/layouts/docs";

import { LogoWithTheme } from "@/components/logo-with-theme";

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
    // No footer with version - keeping it simple like legacy docs
  };
}
