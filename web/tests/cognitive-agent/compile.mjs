import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {stripTypeScriptTypes} from 'node:module';
import {dirname,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';

const root=resolve(import.meta.dirname,'../..');
const output=resolve(import.meta.dirname,'review/src');
const compiled=new Set();

function compile(path){
  const target=resolve(output,path.replace(/\.ts$/,'.mjs'));
  if(compiled.has(path))return target;
  compiled.add(path);
  let code=stripTypeScriptTypes(
    readFileSync(resolve(root,'src',path),'utf8'),
    {mode:'transform'},
  );
  code=code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g,(_,prefix,dependency,suffix)=>{
    compile(resolve(dirname(path),dependency+'.ts').slice(resolve('.').length+1));
    return prefix+dependency+'.mjs'+suffix;
  });
  mkdirSync(dirname(target),{recursive:true});
  writeFileSync(target,code);
  return target;
}

export const source=async path=>import(pathToFileURL(compile(path)));
