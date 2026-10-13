import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { stripTypeScriptTypes } from 'node:module';
import { dirname, resolve, relative } from 'node:path';
import { pathToFileURL } from 'node:url';
const root = resolve(import.meta.dirname, '../../src');
const output = resolve(import.meta.dirname, 'review/src');
const compiled = new Set();
function compile(file) {
  const path = resolve(root, file), target = resolve(output, relative(root, path).replace(/\.ts$/, '.mjs'));
  if (compiled.has(path)) return target;
  compiled.add(path);
  let code = stripTypeScriptTypes(readFileSync(path, 'utf8'), { mode: 'transform' });
  code = code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g, (_, prefix, dependency, suffix) => {
    compile(relative(root, resolve(dirname(path), dependency + '.ts')));
    return prefix + dependency + '.mjs' + suffix;
  });
  mkdirSync(dirname(target), { recursive: true }); writeFileSync(target, code); return target;
}
export const source = async file => import(pathToFileURL(compile(file)));
