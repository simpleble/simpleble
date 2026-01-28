import { execSync } from "child_process";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { XMLParser } from "fast-xml-parser";
import { generateClassMDX, findCompoundFile } from "./utils.js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(__dirname, "..");
const doxyfile = path.join(root, "Doxyfile");
const xmlDir = path.join(root, "_doxygen", "xml");
const outMd = path.join(root, "content", "docs", "simpleble", "api.mdx");

const FRONTMATTER = `---
title: API Reference
description: Architecture overview and API documentation for SimpleBLE.
---

import { ApiClass, ApiMethod, ApiSection } from "@/components/api";

`;

// Classes to document (matches docs/simpleble/api.rst)
const STANDARD_API = [
  "SimpleBLE::Adapter",
  "SimpleBLE::Peripheral",
  "SimpleBLE::Service",
  "SimpleBLE::Characteristic",
  "SimpleBLE::Descriptor",
];

const SAFE_API = ["SimpleBLE::Safe::Adapter", "SimpleBLE::Safe::Peripheral"];

const EXTERNAL_API = ["kvn::bytearray"];

console.log("Building API docs (Doxygen → Custom Parser)...");

// 1. Run Doxygen
console.log("Running Doxygen...");
execSync(`doxygen "${doxyfile}"`, { cwd: root, stdio: "inherit" });
console.log("Doxygen done.");

// 2. Parse index.xml
console.log("Parsing Doxygen XML...");
const xmlParser = new XMLParser({
  ignoreAttributes: false,
  attributeNamePrefix: "@_",
  preserveOrder: true,
  processEntities: true,
});

const indexXmlPath = path.join(xmlDir, "index.xml");
const indexXmlContent = fs.readFileSync(indexXmlPath, "utf-8");
const indexDoc = xmlParser.parse(indexXmlContent);

// 3. Generate MDX
let mdx = FRONTMATTER;
mdx += `# API Reference

The following notes provide an overview of the architecture of some of the higher-level classes in the library, as to facilitate their understanding.

<ApiSection title="Standard API" id="standard-api">

`;

// Process Standard API classes
for (const className of STANDARD_API) {
  const compoundFile = findCompoundFile(indexDoc, className, xmlDir);
  if (compoundFile && fs.existsSync(compoundFile)) {
    mdx += generateClassMDX(compoundFile);
  } else {
    console.warn(`Warning: ${className} not found in XML`);
  }
}

mdx += `</ApiSection>

<ApiSection title="Safe API" id="safe-api">

`;

// Process Safe API classes
for (const className of SAFE_API) {
  const compoundFile = findCompoundFile(indexDoc, className, xmlDir);
  if (compoundFile && fs.existsSync(compoundFile)) {
    mdx += generateClassMDX(compoundFile);
  } else {
    console.warn(`Warning: ${className} not found in XML`);
  }
}

mdx += `</ApiSection>

<ApiSection title="External API" id="external-api">

`;

// Process External API classes
for (const className of EXTERNAL_API) {
  const compoundFile = findCompoundFile(indexDoc, className, xmlDir);
  if (compoundFile && fs.existsSync(compoundFile)) {
    mdx += generateClassMDX(compoundFile);
  } else {
    console.warn(`Warning: ${className} not found in XML`);
  }
}

mdx += `</ApiSection>
`;

// 4. Write output
fs.writeFileSync(outMd, mdx, "utf-8");
console.log(`Written: ${path.relative(root, outMd)}`);
console.log("Done.");
