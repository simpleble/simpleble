import Link from "next/link";
import type { ReactElement } from "react";
import { Github, Linkedin, Twitter, Globe } from "lucide-react";

const footerLinkClassName =
  "text-sm text-fd-muted-foreground transition-colors hover:text-primary";

export const Footer = (): ReactElement => {
  return (
    <footer className="border-t border-fd-border bg-fd-background py-12">
      <div className="mx-auto max-w-[1100px] px-4 sm:px-6 lg:px-8">
        <div className="grid gap-10 sm:grid-cols-2 lg:grid-cols-[1.4fr_1fr_1fr]">
          <div className="space-y-3">
            <p className="font-sora text-base font-semibold text-fd-foreground">
              SimpleBLE
            </p>
            <p className="max-w-sm text-sm leading-relaxed text-fd-muted-foreground">
              A fully supported, cross-platform library for building reliable
              Bluetooth products.
            </p>
          </div>

          <div className="space-y-3">
            <p className="font-sora text-sm font-semibold text-fd-foreground">
              SimpleBLE Store
            </p>
            <p className="text-sm leading-relaxed text-fd-muted-foreground">
              Official apparel and accessories for developers, makers, and IoT
              teams.
            </p>
            <Link
              href="https://simpleble.store/?utm_source=simpleble_docs&utm_medium=website&utm_campaign=docs_footer"
              target="_blank"
              rel="noopener noreferrer"
              className="inline-flex text-sm font-medium text-primary transition-colors hover:text-primary/80"
            >
              Shop the collection →
            </Link>
          </div>

          <nav aria-label="Footer resources" className="space-y-3">
            <p className="font-sora text-sm font-semibold text-fd-foreground">
              Resources
            </p>
            <div className="flex flex-col items-start gap-2">
              <Link href="/licensing" className={footerLinkClassName}>
                Licensing &amp; support
              </Link>
              <Link href="/changelog" className={footerLinkClassName}>
                Changelog
              </Link>
              <Link
                href="https://simpleble.org/contact?utm_source=simpleble_docs&utm_medium=website&utm_campaign=docs_footer"
                target="_blank"
                rel="noopener noreferrer"
                className={footerLinkClassName}
              >
                Contact
              </Link>
            </div>
          </nav>
        </div>

        <div className="mt-10 flex flex-col gap-6 border-t border-fd-border pt-6 md:flex-row md:items-center md:justify-between">
          <div className="flex flex-col gap-2 text-sm text-fd-muted-foreground sm:flex-row sm:flex-wrap sm:items-center sm:gap-x-4">
            <p>
              © Copyright {new Date().getFullYear()} SimpleBLE by{" "}
              <Link
                href="https://californiaopensource.com/?utm_source=simpleble_docs&utm_medium=website&utm_campaign=docs_footer"
                target="_blank"
                rel="noopener noreferrer"
                className="transition-colors hover:text-primary"
              >
                The California Open Source Company
              </Link>
            </p>
            <Link
              href="https://simpleble.org/terms-of-service"
              target="_blank"
              rel="noopener noreferrer"
              className="transition-colors hover:text-primary"
            >
              Terms
            </Link>
            <Link
              href="https://simpleble.org/privacy-policy"
              target="_blank"
              rel="noopener noreferrer"
              className="transition-colors hover:text-primary"
            >
              Privacy
            </Link>
          </div>

          <div className="flex items-center gap-5">
            <Link
              href="https://simpleble.org?utm_source=simpleble_docs&utm_medium=website&utm_campaign=docs_footer"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground transition-colors hover:text-primary"
              aria-label="SimpleBLE website"
            >
              <Globe className="size-5" />
            </Link>
            <Link
              href="https://x.com/LibSimpleBLE"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground transition-colors hover:text-primary"
              aria-label="SimpleBLE on X"
            >
              <Twitter className="size-5" />
            </Link>
            <Link
              href="https://www.linkedin.com/company/simpleble/"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground transition-colors hover:text-primary"
              aria-label="SimpleBLE on LinkedIn"
            >
              <Linkedin className="size-5" />
            </Link>
            <Link
              href="https://github.com/simpleble/simpleble"
              target="_blank"
              rel="noopener noreferrer"
              className="text-fd-muted-foreground transition-colors hover:text-primary"
              aria-label="SimpleBLE on GitHub"
            >
              <Github className="size-5" />
            </Link>
          </div>
        </div>
      </div>
    </footer>
  );
};
