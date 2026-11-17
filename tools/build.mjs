import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';

const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
let compiler=process.env.CC||'gcc';
const args=process.argv.slice(2);
if(args.length){if(args.length!==2||args[0]!=='--cc'){console.error('Usage: node tools/build.mjs [--cc PATH_TO_GCC_CLANG_OR_ZIG]');process.exit(2);}compiler=args[1];}
const zig=/^zig(?:\.exe)?$/i.test(path.basename(compiler));
const windows=process.platform==='win32';
await fs.mkdir(path.join(root,'build'),{recursive:true});
const core=['models','date_utils','input','employee','attendance','payroll','storage'].map(n=>`src/${n}.c`);
const programs=[['chamcong',[...core,'src/main.c','src/ui.c','src/ui_display.c','src/ui_employee.c','src/ui_attendance.c','src/ui_payroll.c','src/search_statistics.c']],['test_core',[...core,'tests/test_core.c']],['test_linked_list',[...core,'tests/test_linked_list.c']]];
const report={compiler,standard:'C11',warningsAsErrors:true,results:[]};
for(const [name,files] of programs){
  const output=`build/${name}${windows?'.exe':''}`;
  const argv=[...(zig?['cc']:[]),'-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-Iinclude',...files,'-o',output];
  const run=spawnSync(compiler,argv,{cwd:root,encoding:'utf8',windowsHide:true,timeout:180000});
  report.results.push({target:name,status:run.status,error:run.error?.message,stdout:run.stdout,stderr:run.stderr});
  if(run.error||run.status!==0){console.error(run.error?.message||run.stderr);await fs.writeFile(path.join(root,'build/build_report.json'),JSON.stringify(report,null,2));process.exit(1);}
  console.log(`Built ${output}`);
}
await fs.writeFile(path.join(root,'build/build_report.json'),JSON.stringify(report,null,2));
