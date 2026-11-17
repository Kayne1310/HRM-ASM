import fs from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const runtime=await fs.mkdtemp(path.join(os.tmpdir(),'chamcong-stress-'));
const name=process.platform==='win32'?'chamcong.exe':'chamcong';
const exe=path.join(runtime,name);await fs.copyFile(path.join(root,'build',name),exe);
const data=path.join(runtime,'data');await fs.mkdir(data);
let employees='EMPLOYEES|1\n';
for(let i=1;i<=200;i++)employees+=`NV${String(i).padStart(3,'0')}|Nhân viên ${i}|Phòng ${i%5}|Nhân viên|2020-01-01||10000000|1000000|1\n`;
await fs.writeFile(path.join(data,'employees.txt'),employees);
await fs.writeFile(path.join(data,'payroll_inputs.txt'),'PAYROLL_INPUTS|1\n');
const records=[];const date=new Date(Date.UTC(2020,0,1));
while(records.length<150001){if(date.getUTCDay()!==0){for(let i=1;i<=200&&records.length<150001;i++)records.push(`NV${String(i).padStart(3,'0')}|${date.toISOString().slice(0,10)}|1|16|0|`);}date.setUTCDate(date.getUTCDate()+1);}
const attendance=path.join(data,'attendance.txt');await fs.writeFile(attendance,'ATTENDANCE|1\n'+records.slice(0,150000).join('\n')+'\n');
function run(){const start=performance.now();const r=spawnSync(exe,['--data',data,'--check'],{cwd:runtime,encoding:'utf8',windowsHide:true,timeout:60000});return{status:r.status,milliseconds:Math.round(performance.now()-start),output:(r.stdout||'')+(r.stderr||'')}}
const atLimit=run();await fs.appendFile(attendance,records[150000]+'\n');const overLimit=run();
await fs.writeFile(path.join(data,'employees.txt'),employees+'OVER|Vượt giới hạn|Phòng 1|NV|2020-01-01||1000000|0|1\n');const employeeLimit=run();
const report={date:new Date().toISOString(),platform:process.platform,atLimit,overLimit,employeeLimit,passed:atLimit.status===0&&atLimit.output.includes('150000 công')&&overLimit.status!==0&&employeeLimit.status!==0};
await fs.writeFile(path.join(root,'build/stress_report.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report,null,2));if(!report.passed)process.exitCode=1;
