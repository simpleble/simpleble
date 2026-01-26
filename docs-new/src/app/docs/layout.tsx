import { source } from "@/lib/source";
import { DocsLayout } from "fumadocs-ui/layouts/docs";
import { baseOptions, docsSidebarOptions } from "@/lib/layout.shared";
import { SidebarHoverEffect } from "@/components/sidebar-hover-effect";

export default function Layout({ children }: LayoutProps<"/docs">) {
  return (
    <DocsLayout
      tree={source.getPageTree()}
      themeSwitch={{ enabled: false }}
      sidebar={docsSidebarOptions()}
      {...baseOptions()}
    >
      <SidebarHoverEffect />
      {children}
    </DocsLayout>
  );
}
