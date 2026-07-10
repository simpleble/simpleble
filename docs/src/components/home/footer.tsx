import Link from "next/link";
import type { ReactElement } from "react";
import { Github, Linkedin, Twitter, Globe } from "lucide-react";

export const Footer = (): ReactElement => {
  return (
    <footer className="border-t border-fd-border bg-fd-background py-12">
      <div className="mx-auto max-w-[1100px] px-4 sm:px-6 lg:px-8">
        <div className="flex flex-col items-center justify-between gap-8 md:flex-row">
          <div className="flex flex-col items-center gap-4 md:items-start">
            <p className="text-sm text-fd-muted-foreground font-sora">
              © Copyright {new Date().getFullYear()} SimpleBLE by The California Open Source Company
            </p>
          </div>

          <div className="flex items-center gap-6">
            <Link
              href="https://www.simpleble.org"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground hover:text-primary transition-colors"
              aria-label="Website"
            >
              <Globe className="size-5" />
            </Link>
            <Link
              href="https://x.com/LibSimpleBLE"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground hover:text-primary transition-colors"
              aria-label="X (Twitter)"
            >
              <Twitter className="size-5" />
            </Link>
            <Link
              href="https://www.linkedin.com/company/simpleble/"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground hover:text-primary transition-colors"
              aria-label="LinkedIn"
            >
              <Linkedin className="size-5" />
            </Link>
            <Link
              href="https://github.com/simpleble/simpleble"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground hover:text-primary transition-colors"
              aria-label="GitHub"
            >
              <Github className="size-5" />
            </Link>
          </div>
        </div>
      </div>
    </footer>
  );
};
