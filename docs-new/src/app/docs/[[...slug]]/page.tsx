import { getPageImage, source } from "@/lib/source";
import {
  DocsBody,
  DocsDescription,
  DocsPage,
  DocsTitle,
} from "fumadocs-ui/layouts/docs/page";
import { notFound } from "next/navigation";
import { getMDXComponents } from "@/mdx-components";
import type { Metadata } from "next";
import { createRelativeLink } from "fumadocs-ui/mdx";
import { LLMCopyButton, ViewOptions } from "@/components/ai/page-actions";

export default async function Page(props: PageProps<"/docs/[[...slug]]">) {
  const params = await props.params;
  const page = source.getPage(params.slug);
  if (!page) notFound();

  const MDX = page.data.body;
  const gitConfig = {
    user: "username",
    repo: "repo",
    branch: "main",
  };

  return (
    <DocsPage
      toc={page.data.toc}
      full={page.data.full}
      tableOfContent={{
        style: "normal",
      }}
    >
      <div className="mb-12">
        <DocsTitle className="text-4xl sm:text-5xl font-sora font-bold tracking-tight mb-4">
          {page.data.title}
        </DocsTitle>
        <DocsDescription className="text-lg text-fd-muted-foreground leading-relaxed mb-8">
          {page.data.description}
        </DocsDescription>
        <div className="flex flex-row gap-4 items-center border-y py-4 border-fd-border/50">
          <LLMCopyButton markdownUrl={`${page.url}.mdx`} />
          <ViewOptions
            markdownUrl={`${page.url}.mdx`}
            githubUrl={`https://github.com/${gitConfig.user}/${gitConfig.repo}/blob/${gitConfig.branch}/docs/content/docs/${page.path}`}
          />
        </div>
      </div>
      <DocsBody className="prose-lg prose-fd-primary max-w-none">
        <MDX
          components={getMDXComponents({
            a: createRelativeLink(source, page),
          })}
        />
      </DocsBody>
    </DocsPage>
  );
}

export async function generateStaticParams() {
  return source.generateParams();
}

export async function generateMetadata(
  props: PageProps<"/docs/[[...slug]]">,
): Promise<Metadata> {
  const params = await props.params;
  const page = source.getPage(params.slug);
  if (!page) notFound();

  return {
    title: page.data.title,
    description: page.data.description,
    openGraph: {
      images: getPageImage(page).url,
    },
  };
}
