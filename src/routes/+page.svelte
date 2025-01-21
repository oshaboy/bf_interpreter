<script lang=ts>
import * as ansiHTML from "ansi-html";
import codepage from "codepage"
const {utils}=codepage;
const encodings=[
	{name: "UTF-8 (Default)",         codepage: 65001},
	{name: "Codepage 437 (OEM/VGA)",  codepage: 437},
	{name: "ISO-8859-1 (West Europe)",codepage: 28591},
	{name: "ISO-8859-2 (East Europe)",codepage: 28592},
	{name: "ISO-8859-3 (Maltese)",    codepage: 28593},
	{name: "ISO-8859-4 (Scandinavia)",codepage: 28594},
	{name: "ISO-8859-5 (Cyrillic)",   codepage: 28595},
	{name: "ISO-8859-6 (Arabic)",codepage: 28596},
	{name: "ISO-8859-7 (Greek)",codepage: 28597},
	{name: "ISO-8859-8 (Hebrew)",codepage: 28598},
	{name: "ISO-8859-9 (Turkish)",codepage: 28599},
	{name: "ISO-8859-10 (Nordic)",codepage: 28600},
	{name: "ISO-8859-11 (Thai)",codepage: 28601},
	{name: "ISO-8859-13 (Baltic)",codepage: 28603},
	{name: "ISO-8859-14 (Celtic)",codepage: 28604},
	{name: "ISO-8859-15 (West Europe)",codepage: 28605},
	{name: "ISO-8859-16 (East Europe)",codepage: 28606},
	{name: "KOI-8-R",codepage:20866},
	{name: "KOI-8-U",codepage:21866},
	{name: "Shift JIS (Japanese)",codepage: 932},
	{name: "Windows 1250 (East Europe)",codepage: 1250},
	{name: "Windows 1251 (Cyrillic)",codepage: 1251},
	{name: "Windows 1252 (West Europe)",codepage: 1252},
	{name: "Windows 1253 (Greek)",codepage: 1253},
	{name: "Windows 1254 (Turkish)",codepage: 1250},
	{name: "Windows 1255 (Hebrew)",codepage: 1255},
	{name: "Windows 1256 (Arabic)",codepage: 1256},
	{name: "Windows 1257 (Baltic)",codepage: 1257},
	{name: "Windows 1258 (Vietnamese)",codepage: 1258},
	{name: "EBCDIC 🤮", codepage: 37}
];
const bf_initial_state={
	tape_head: 16,
	tape: new Array(32).fill(0) as number[],
	program : "",
	ip : 0,
	output: [] as number[],
	stack : [] as number[],
	input_buffer: [] as number[]
};
let bfstate = $state({...bf_initial_state});
$inspect(bfstate);
let current_encoding_index = $state(0);
let input_text=$state("");
let output_mode=$state(0);
let cell_size_index=$state(0);
function nearby_cells() : number[]{
	const head_coarse=Math.floor(bfstate.tape_head/16)*16;
	let array_copy=[...bfstate.tape];
	return array_copy.slice(head_coarse, head_coarse+15);
}
function step() : boolean {
	const command=bfstate.program.charAt(bfstate.ip);
	console.log(command);
	if (command=="+"){
		bfstate.tape[bfstate.tape_head]++;
		if (bfstate.tape[bfstate.tape_head]==256){
			bfstate.tape[bfstate.tape_head]=0;
		}
	}
	else if (command=="-"){
		bfstate.tape[bfstate.tape_head]--;
		if (bfstate.tape[bfstate.tape_head]==-1){
			bfstate.tape[bfstate.tape_head]=255;
		}
	}
	else if (command=="<"){
		bfstate.tape_head--;
		if (bfstate.tape_head<0){
			const len=bfstate.tape.length;
			const empty=new Array(len*3).fill(0);
			bfstate.tape_head=len*3-1;
			bfstate.tape=[...empty,...bfstate.tape];

		}
	}
	else if (command==">"){
		bfstate.tape_head++;
		if (bfstate.tape_head>=bfstate.tape.length){
			const len=bfstate.tape.length;
			const empty=new Array(len*3).fill(0);
			bfstate.tape=[...bfstate.tape,...empty];
		}
	}
	else if (command=="."){
		bfstate.output.push(bfstate.tape[bfstate.tape_head]);
	}
	else if (command==","){
		if (bfstate.input_buffer.length==0){
			if (input_text.length == 0){
				bfstate.input_buffer=[0];
			} else {
				const codepoint=input_text.codePointAt(0)!;
				const encoding=encodings[current_encoding_index].codepage;
				const bytes=utils.encode(encoding,String.fromCodePoint(codepoint));
				bfstate.input_buffer=[...(bytes as number[])];
				if (codepoint>=0x10000){
					input_text=input_text.slice(2);
				} else {
					input_text=input_text.slice(1);
				}
			}
		} 
		bfstate.tape[bfstate.tape_head]=bfstate.input_buffer[0];
		bfstate.input_buffer=bfstate.input_buffer.slice(1);
	}
	else if (command=="["){
		if (bfstate.tape[bfstate.tape_head] == 0) {
			bfstate.ip++;
			let bracket_count=1;
			while(bracket_count>0){
				if (bfstate.program[bfstate.ip] == "["){
					bracket_count++;
				} else if (bfstate.program[bfstate.ip] == "]"){
					bracket_count--;
					break;
				}
				bfstate.ip++;
			}
		} else {
			bfstate.stack.push(bfstate.ip);
		}
		
	}
	else if (command=="]"){
		if (bfstate.tape[bfstate.tape_head] == 0) {
			bfstate.stack.pop()
		} else {
			bfstate.ip=bfstate.stack.at(-1)!;
		}
			
	}
	
	
	if(bfstate.ip < bfstate.program.length){
		bfstate.ip++;
	} else {
		return true;
	}
	return false;
}
function reset(){
	bfstate=bf_initial_state;
	let progstring=
		(document.getElementById("codebox")! as HTMLTextAreaElement).
		value;
	/*Check if program is valid*/
	let bracket_count=0;
	let isvalid=true;
	for (let i=0; i<progstring.length; i++){
		let c=progstring.charAt(i);
		if (c=="["){
			bracket_count++;
		} else if (c=="]"){
			bracket_count--;
			if (bracket_count<0){
				isvalid=false;
				break;
			}
		} else if (c==" "){
			progstring=progstring.substring(0,i) + "\u{a0}" + progstring.substring(i+1);
		}
	}
	if (bracket_count != 0)
		isvalid=false;
	
	if (progstring != "" && isvalid)
		bfstate.program = progstring;

		
}
function getProgramAsString() : string{
	if (bfstate.program == ""){
		return "\u{a0}";
	} else {
		return bfstate.program;
	}
}


function positionProgramCursor() : string {
	const row=Math.floor(bfstate.ip/64);
	return `translate(${bfstate.ip%64}ch,${row*2}em)`;
}

function get_formatted_output(): string{
	const encoding=encodings[current_encoding_index].codepage;
	let output=utils.decode(encoding,Uint8Array.from(bfstate.output));
	switch(output_mode) {
		case 1:
			console.log(ansiHTML);
			output=ansiHTML.default(output);
		case 0:
		case 2:
			break;
		default:
			return "unimplemented";

	}
	return output;
}

</script>
<main>
<div class="main">
	<div class="codebox-area">
		<textarea id="codebox" class="codebox"></textarea>
		<div class="button-area">
			<button onclick={()=>{while(!step());}}>run</button>
			<button onclick={step}>step</button>
			<button onclick={reset}>reset</button>
		</div>
		<textarea class="input_box"
			bind:value={
			()=>input_text,
			(value)=>{input_text=value}
			}
		></textarea>
	</div>
	
	<div class="debug-area">
		<div class="program-area">
		<div class="program">
		{getProgramAsString()}
		</div>
		<div class="cursor"
		style:transform={positionProgramCursor()}
		>^</div>
		</div>
		<div class="tape-view">
		{#each nearby_cells() as cell}
		<div class="cell">{cell}</div>
		{/each}
		</div>
		<div
		style:transform={`translateX(${(bfstate.tape_head%16)*2+1}em)`}
		>^</div>
	</div>

	<div class="output-area">
		<div class="output">
			{#if output_mode==0}
{get_formatted_output()}
			{:else}
			{@html get_formatted_output()}
			{/if}
		</div>
		
		<select class="encoding-selector"
			onchange={e=>current_encoding_index=(e.target! as HTMLSelectElement).selectedIndex}>
			{#each encodings as encoding}
			<option>{encoding.name}</option>
			{/each}
		</select>
		<select onchange={e=>output_mode=(e.target! as HTMLSelectElement).selectedIndex}>
			<option>Plaintext</option>
			<option>ECMA-48 (ANSI)</option>
			<option>HTML</option>
		</select>
	</div>

</div>
</main>
<style>
.codebox-area {
	display: flex;
	flex-direction: column; 
}
.output-area {
	display: flex;
	flex-direction: column; 
	margin-left:10px;
}
.codebox {
	width: 20em;
	height: 20em;
}

button {
	width:3em;
}

.button-area {
	display: flex;
	position:relative;
	flex-direction: row; 
	margin-right: 0;
	margin-left: auto;
	margin-top: auto;
}
.output {
	min-width: 20em;
	min-height: 10em;
	width: auto;
	height: auto;
	background-color: black;
	color: white;
	display: flex;
	
}

.main {
	display:flex;
	flex-direction: row;
}
.encoding-selector {
	display:flex;
}
.debug-area {
	margin-left: 10px;
	height: 20em;
	background-color: lightgray;
	font-family: monospace;
	display:flex;
	flex-direction: column;
	height: auto;
	min-height: 10em;
}
.cursor{ 
	position:absolute;
	top:2em;
	font-size:1em;
}
.program{
	width: 64ch;
	line-height: 2em;
}
.program-area{ 
	display:flex;
	font-size:1em;
	margin-left: 0.5em;
	margin-right: 0.5em;
}
.cell{
	font-size:1em;
	width:2em;
	height:2em;
	border: 1px black solid;
	text-align: center;
}
.tape-view{
	display:flex;
	flex-direction: row;
	margin-bottom: 1em;
	margin-top: auto;
	margin-left: 0.5em;
	margin-right: 0.5em;
}
</style>