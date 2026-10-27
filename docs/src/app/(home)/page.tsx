import type { Metadata } from "next";

import { DocsHome } from "@/components/home/docs-home";

export const metadata: Metadata = {
  title: {
    absolute: "SimpleBLE Docs: Cross-Platform Bluetooth Low Energy Library",
  },
  alternates: {
    canonical: "/",
  },
};

export default function HomePage() {
  return <DocsHome />;
}
