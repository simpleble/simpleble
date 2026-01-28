#!/usr/bin/env node
/**
 * Custom Doxygen XML → MDX parser.
 * Uses preserveOrder to correctly handle mixed content (text + inline elements).
 */
import { XMLParser } from "fast-xml-parser";
import fs from "fs";
import path from "path";

const parser = new XMLParser({
  ignoreAttributes: false,
  attributeNamePrefix: "@_",
  parseAttributeValue: true,
  trimValues: true,
  textNodeName: "#text",
  cdataPropName: "__cdata",
  preserveOrder: true,
  processEntities: true,
});

// ============================================================================
// Callout Mapping Constants
// ============================================================================

/**
 * Map Doxygen simplesect kinds to Fumadocs Callout types.
 */
const SIMPLESECT_TO_CALLOUT = {
  'note': 'info',
  'warning': 'warn',
  'attention': 'warn',
  'remark': 'idea',
  // 'return' is handled separately, not as a callout
};

/**
 * Map informal note prefixes to Callout types and titles.
 */
const INLINE_NOTE_PATTERNS = [
  { pattern: /NOTE:\s*/gi, type: 'info', title: 'Note' },
  { pattern: /WARNING:\s*/gi, type: 'warn', title: 'Warning' },
  { pattern: /IMPORTANT:\s*/gi, type: 'warn', title: 'Important' },
  { pattern: /TODO:\s*/gi, type: 'idea', title: 'Todo' },
  { pattern: /ATTENTION:\s*/gi, type: 'warn', title: 'Attention' },
];

// ============================================================================
// Helper Functions for preserveOrder structure
// ============================================================================

/**
 * Get text content from a preserveOrder text node.
 */
function getText(node) {
  if (node && node["#text"]) {
    return String(node["#text"]);
  }
  return "";
}

/**
 * Get attributes from a preserveOrder node structure.
 */
function getAttrs(item) {
  return item[":@"] || {};
}

/**
 * Find first child element with given tag name.
 */
function findFirst(arr, tagName) {
  if (!Array.isArray(arr)) return null;
  for (const item of arr) {
    if (item[tagName]) return item[tagName];
  }
  return null;
}

/**
 * Find all child elements with given tag name.
 */
function findAll(arr, tagName) {
  if (!Array.isArray(arr)) return [];
  const results = [];
  for (const item of arr) {
    if (item[tagName]) results.push(item[tagName]);
  }
  return results;
}

/**
 * Convert Doxygen refid to MDX anchor ID.
 */
function convertRefidToAnchor(refid) {
  if (!refid || typeof refid !== "string") return null;
  
  if (!refid.startsWith("class_")) return null;
  if (refid.match(/class_.*_1[a-f0-9]{30,}/)) return null;
  
  let id = refid.substring(6);
  id = id.replace(/_1_1/g, "::NS::");
  id = id.replace(/_/g, "");
  id = id.replace(/::NS::/g, "-");
  id = id.toLowerCase();
  
  return `#${id}`;
}

/**
 * Extract text from preserveOrder array, handling inline elements correctly.
 * Note: simplesect elements are intentionally skipped as they're handled separately.
 */
function extractText(arr) {
  if (!Array.isArray(arr)) return "";
  
  const parts = [];
  
  for (const item of arr) {
    // Skip simplesect - handled by parsePara/parseDescription
    if (item["simplesect"]) {
      continue;
    }
    
    // Text nodes
    if (item["#text"]) {
      parts.push(getText(item));
    }
    
    // Ref elements (internal links)
    if (item["ref"]) {
      const attrs = getAttrs(item);
      const refid = attrs["@_refid"];
      const text = extractText(item["ref"]);
      
      if (refid && text) {
        const anchorId = convertRefidToAnchor(refid);
        if (anchorId) {
          parts.push(`[${text}](${anchorId})`);
        } else {
          parts.push(text);
        }
      } else if (text) {
        parts.push(text);
      }
    }
    
    // Inline code
    if (item["computeroutput"]) {
      const text = extractText(item["computeroutput"]);
      if (text) parts.push(`\`${text}\``);
    }
    
    // Emphasis
    if (item["emphasis"]) {
      const text = extractText(item["emphasis"]);
      if (text) parts.push(`*${text}*`);
    }
    
    // Bold
    if (item["bold"]) {
      const text = extractText(item["bold"]);
      if (text) parts.push(`**${text}**`);
    }
    
    // Nested para
    if (item["para"]) {
      const text = extractText(item["para"]);
      if (text) parts.push(text);
    }
  }
  
  let result = parts.join(" ").replace(/\s+/g, " ");
  
  // Clean up spacing issues
  result = result
    .replace(/\s+\./g, ".") // Remove space before period
    .replace(/\s+,/g, ",") // Remove space before comma
    .replace(/\(\s+/g, "(") // Remove space after (
    .replace(/\s+\)/g, ")") // Remove space before )
    .trim();
  
  return result;
}

/**
 * Check if text starts with an informal note pattern and extract it.
 * Returns { isNote: true, type, title, content } or { isNote: false }
 */
function detectInlineNote(text) {
  if (!text) return { isNote: false };
  
  for (const { pattern, type, title } of INLINE_NOTE_PATTERNS) {
    if (pattern.test(text)) {
      const content = text.replace(pattern, '').trim();
      return { isNote: true, type, title, content };
    }
  }
  
  return { isNote: false };
}

/**
 * Split text by informal note patterns to handle multiple notes.
 * Returns array of { isNote, type, title, content } objects.
 */
function splitByInlineNotes(text) {
  if (!text) return [];
  
  // Find all note matches with their positions
  const matches = [];
  
  for (const notePattern of INLINE_NOTE_PATTERNS) {
    // Create a new regex with the same pattern (to reset lastIndex)
    const regex = new RegExp(notePattern.pattern.source, 'gi');
    let match;
    
    while ((match = regex.exec(text)) !== null) {
      matches.push({
        index: match.index,
        length: match[0].length,
        type: notePattern.type,
        title: notePattern.title
      });
    }
  }
  
  // Sort matches by position
  matches.sort((a, b) => a.index - b.index);
  
  if (matches.length === 0) {
    // No notes found - return as regular text
    return [{ isNote: false, content: text.trim() }];
  }
  
  const results = [];
  let lastIndex = 0;
  
  for (let i = 0; i < matches.length; i++) {
    const match = matches[i];
    
    // Add any text before this note
    if (match.index > lastIndex) {
      const beforeText = text.substring(lastIndex, match.index).trim();
      if (beforeText) {
        results.push({ isNote: false, content: beforeText });
      }
    }
    
    // Determine where this note's content ends
    const contentStart = match.index + match.length;
    const contentEnd = i + 1 < matches.length ? matches[i + 1].index : text.length;
    const content = text.substring(contentStart, contentEnd).trim();
    
    if (content) {
      results.push({
        isNote: true,
        type: match.type,
        title: match.title,
        content
      });
    }
    
    lastIndex = contentEnd;
  }
  
  return results.filter(r => r.content); // Remove empty segments
}

/**
 * Generate Callout component syntax.
 */
function generateCallout(type, title, content) {
  // Escape content for JSX - but preserve markdown formatting
  const escapedContent = content
    .replace(/\\/g, "\\\\")
    .replace(/"/g, '\\"');
  
  return `<Callout type="${type}" title="${title}">\n${content}\n</Callout>`;
}

/**
 * Parse a single para element, checking for simplesect children.
 * Returns an object: { text: string, callouts: Array<{type, title, content}> }
 */
function parsePara(paraArray) {
  if (!Array.isArray(paraArray) || paraArray.length === 0) {
    return { text: "", callouts: [] };
  }
  
  const callouts = [];
  const textParts = [];
  
  for (const item of paraArray) {
    // Check for simplesect (formal notes like @note, @warning)
    if (item["simplesect"]) {
      const simplesectArray = item["simplesect"];
      const attrs = getAttrs(item);
      const kind = attrs["@_kind"];
      
      // Only convert certain kinds to callouts
      if (SIMPLESECT_TO_CALLOUT[kind]) {
        const calloutType = SIMPLESECT_TO_CALLOUT[kind];
        const title = kind.charAt(0).toUpperCase() + kind.slice(1);
        
        // Extract content from simplesect's para elements
        const contentParas = findAll(simplesectArray, "para");
        const content = contentParas.map(p => extractText(p)).filter(Boolean).join("\n\n");
        
        if (content) {
          callouts.push({ type: calloutType, title, content });
        }
      }
    } else {
      // Regular text content - will be processed by extractText
      // We'll collect these items to pass to extractText
    }
  }
  
  // Extract regular text (non-simplesect elements)
  const regularText = extractText(paraArray);
  
  return { text: regularText, callouts };
}

/**
 * Parse description node (briefdescription or detaileddescription).
 * Returns object with { text, callouts } where callouts are separate from text.
 * This allows callouts to be rendered as JSX components, not escaped strings.
 */
function parseDescription(descArray) {
  if (!Array.isArray(descArray) || descArray.length === 0) {
    return { text: "", callouts: [] };
  }
  
  const paras = findAll(descArray, "para");
  const textParts = [];
  const allCallouts = [];
  
  for (const para of paras) {
    const { text, callouts } = parsePara(para);
    
    // Check if text has inline note patterns (might be multiple)
    if (text) {
      const segments = splitByInlineNotes(text);
      
      for (const segment of segments) {
        if (segment.isNote) {
          // Add as callout
          allCallouts.push({
            type: segment.type,
            title: segment.title,
            content: segment.content
          });
        } else {
          // Regular text
          textParts.push(segment.content);
        }
      }
    }
    
    // Add any formal callouts from simplesect
    allCallouts.push(...callouts);
  }
  
  return {
    text: textParts.filter(Boolean).join("\n\n").trim(),
    callouts: allCallouts
  };
}

/**
 * Legacy function for backward compatibility.
 * Returns just the text part, used where callouts aren't supported.
 */
function parseDescriptionText(descArray) {
  const result = parseDescription(descArray);
  return result.text;
}

/**
 * Escape string for JSX attribute.
 */
function escapeJsx(text) {
  if (!text) return "";
  return text
    .replace(/\\/g, "\\\\")
    .replace(/"/g, '\\"')
    .replace(/\n/g, " ")
    .replace(/\r/g, "")
    .trim();
}

/**
 * Extract return type from function definition.
 */
function extractReturnType(definition, className, name) {
  if (!definition || !name) return "";
  
  const nameIndex = definition.lastIndexOf(name);
  if (nameIndex === -1) return "";
  
  let beforeName = definition.substring(0, nameIndex).trim();
  
  if (beforeName.endsWith("::")) {
    beforeName = beforeName.substring(0, beforeName.length - 2).trim();
  }
  
  const parts = beforeName.split(/\s+/);
  if (parts.length === 1) return "";
  
  let depth = 0;
  let lastSpaceOutsideBrackets = -1;
  for (let i = 0; i < beforeName.length; i++) {
    if (beforeName[i] === "<") depth++;
    else if (beforeName[i] === ">") depth--;
    else if (beforeName[i] === " " && depth === 0) {
      lastSpaceOutsideBrackets = i;
    }
  }
  
  if (lastSpaceOutsideBrackets !== -1) {
    return beforeName.substring(0, lastSpaceOutsideBrackets).trim();
  }
  
  return beforeName;
}

/**
 * Format method signature.
 */
function formatSignature(definition, argsstring, attrs) {
  let sig = `${definition}${argsstring}`.trim();
  
  // Add virtual/static prefixes
  if (attrs["@_virt"] === "virtual" && !sig.startsWith("virtual")) {
    sig = "virtual " + sig;
  }
  if (attrs["@_static"] === "yes" && !sig.startsWith("static")) {
    sig = "static " + sig;
  }
  
  return sig;
}

/**
 * Generate MDX for a single class.
 */
export function generateClassMDX(compoundXmlPath) {
  const xmlContent = fs.readFileSync(compoundXmlPath, "utf-8");
  const docArray = parser.parse(xmlContent);
  
  const doxygenNode = findFirst(docArray, "doxygen");
  if (!doxygenNode) return "";
  
  const compounddefArray = findFirst(doxygenNode, "compounddef");
  if (!compounddefArray) return "";
  
  // Extract class info
  const compoundnameNode = findFirst(compounddefArray, "compoundname");
  const className = compoundnameNode ? extractText(compoundnameNode) : "Unknown";
  
  const briefdescNode = findFirst(compounddefArray, "briefdescription");
  const detaileddescNode = findFirst(compounddefArray, "detaileddescription");
  
  const briefResult = briefdescNode ? parseDescription(briefdescNode) : { text: "", callouts: [] };
  const detailedResult = detaileddescNode ? parseDescription(detaileddescNode) : { text: "", callouts: [] };
  
  // Generate class ID
  const classId = className
    .toLowerCase()
    .replace(/::/g, "-")
    .replace(/[^a-z0-9-]/g, "");
  
  let mdx = `## ${className} [#${classId}] [toc]\n\n`;
  mdx += `<ApiClass name="${escapeJsx(className)}"`;
  if (briefResult.text) mdx += ` brief="${escapeJsx(briefResult.text)}"`;
  if (detailedResult.text) mdx += ` detailed="${escapeJsx(detailedResult.text)}"`;
  mdx += `>\n\n`;
  
  // Add callouts from class description if any
  for (const callout of detailedResult.callouts) {
    mdx += generateCallout(callout.type, callout.title, callout.content) + "\n\n";
  }
  
  // Process sections
  const sectiondefItems = compounddefArray.filter(item => item.sectiondef);
  
  for (const item of sectiondefItems) {
    const sectiondefArray = item.sectiondef;
    const attrs = getAttrs(item);
    const kind = attrs["@_kind"];
    
    if (kind === "public-func" || kind === "public-static-func") {
      const sectionTitle =
        kind === "public-static-func"
          ? "Public Static Functions"
          : "Public Functions";
      
      mdx += `### ${sectionTitle} [!toc]\n\n`;
      
      const memberdefItems = sectiondefArray.filter(item => item.memberdef);
      
      for (const memberItem of memberdefItems) {
        const memberdefArray = memberItem.memberdef;
        const memberAttrs = getAttrs(memberItem);
        const memberKind = memberAttrs["@_kind"];
        
        if (memberKind !== "function") continue;
        
        const nameNode = findFirst(memberdefArray, "name");
        const definitionNode = findFirst(memberdefArray, "definition");
        const argsstringNode = findFirst(memberdefArray, "argsstring");
        
        const name = nameNode ? extractText(nameNode) : "unknown";
        const definition = definitionNode ? extractText(definitionNode) : "";
        const argsstring = argsstringNode ? extractText(argsstringNode) : "()";
        
        // Remove class name from definition to get cleaner signature
        let cleanDefinition = definition.replace(className + "::", "");
        const sig = formatSignature(cleanDefinition, argsstring, memberAttrs);
        
        const memberBriefdescNode = findFirst(memberdefArray, "briefdescription");
        const memberDetaileddescNode = findFirst(memberdefArray, "detaileddescription");
        
        const memberBriefResult = memberBriefdescNode ? parseDescription(memberBriefdescNode) : { text: "", callouts: [] };
        const memberDetailedResult = memberDetaileddescNode ? parseDescription(memberDetaileddescNode) : { text: "", callouts: [] };
        
        const params = [];
        const paramItems = memberdefArray.filter(item => item.param);
        
        for (const paramItem of paramItems) {
          const paramArray = paramItem.param;
          
          const declnameNode = findFirst(paramArray, "declname");
          const typeNode = findFirst(paramArray, "type");
          
          const paramName = declnameNode ? extractText(declnameNode) : "";
          const paramType = typeNode ? extractText(typeNode) : "";
          
          if (paramName && paramType) {
            params.push({ name: paramName, type: paramType });
          }
        }
        
        // Check if method has callouts - if so, use children syntax
        const hasCallouts = memberDetailedResult.callouts.length > 0;
        
        if (hasCallouts) {
          // Use children syntax for callouts
          mdx += `<ApiMethod\n`;
          mdx += `  signature="${escapeJsx(sig)}"\n`;
          if (memberBriefResult.text) mdx += `  brief="${escapeJsx(memberBriefResult.text)}"\n`;
          if (memberDetailedResult.text) mdx += `  detailed="${escapeJsx(memberDetailedResult.text)}"\n`;
          if (params.length > 0) {
            mdx += `  parameters={${JSON.stringify(params)}}\n`;
          }
          mdx += `>\n\n`;
          
          // Add callouts as children
          for (const callout of memberDetailedResult.callouts) {
            mdx += generateCallout(callout.type, callout.title, callout.content) + "\n\n";
          }
          
          mdx += `</ApiMethod>\n\n`;
        } else {
          // Use self-closing syntax (no callouts)
          mdx += `<ApiMethod\n`;
          mdx += `  signature="${escapeJsx(sig)}"\n`;
          if (memberBriefResult.text) mdx += `  brief="${escapeJsx(memberBriefResult.text)}"\n`;
          if (memberDetailedResult.text) mdx += `  detailed="${escapeJsx(memberDetailedResult.text)}"\n`;
          if (params.length > 0) {
            mdx += `  parameters={${JSON.stringify(params)}}\n`;
          }
          mdx += `/>\n\n`;
        }
      }
    }
  }
  
  mdx += `</ApiClass>\n\n`;
  return mdx;
}

/**
 * Find compound XML file path from index by class name.
 * indexDoc is the parsed array from preserveOrder parser.
 */
export function findCompoundFile(indexDoc, className, xmlDir) {
  // indexDoc is an array, find the doxygenindex element
  const doxygenindexItem = indexDoc.find(item => item.doxygenindex);
  if (!doxygenindexItem) return null;
  
  const doxygenindexArray = doxygenindexItem.doxygenindex;
  if (!Array.isArray(doxygenindexArray)) return null;
  
  // Each item in doxygenindexArray might have a compound
  for (const item of doxygenindexArray) {
    if (item.compound) {
      const compoundArray = item.compound;
      const attrs = getAttrs(item);
      
      // Find the name inside the compound array
      const nameNode = findFirst(compoundArray, "name");
      const name = nameNode ? extractText(nameNode) : "";
      
      if (name === className) {
        const refid = attrs["@_refid"];
        if (refid) {
          return path.join(xmlDir, `${refid}.xml`);
        }
      }
    }
  }
  
  return null;
}
