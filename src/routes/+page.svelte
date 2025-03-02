<script lang=ts>
import * as ansiHTML from "ansi-html";
import codepage from "codepage";
const {utils}=codepage;

function codepage_number(encoding :number) {
	return {
		encoder: (str : string) => {return utils.encode(encoding,str)},
		decoder: (buf : Uint8Array) => {return utils.decode(encoding,buf)}
	}
}
function load_file(){
	if (file != undefined){
		const reader=new FileReader();
		reader.onload=(e)=>{
			if (e.target != null){
				switch (cell_size_index){
					case eight_bit_mode:
					bfstate.input_buffer=[...new Uint8Array(e.target.result as ArrayBuffer)];
					break;

					case sixteen_bit_mode:
					bfstate.input_buffer=[...new Uint16Array(e.target.result as ArrayBuffer)];
					break;

					case bigint_bit_mode:
					bfstate.input_buffer=[...new Uint32Array(e.target.result as ArrayBuffer)];
					break;
				}
			}
		}
		reader.readAsArrayBuffer(file[0]);

	}
}
const encodings=[
	{name: "UTF-8 (Default)", 				  ...codepage_number(65001)},
	{
		name: "Ignore Non Ascii",
		encoder: (str:string)=>{
			let bytes=[] as number[];
			for (let i=0; i<str.length; i++){
				let codepoint=str.codePointAt(i)!;
				if (codepoint < 0x80 && codepoint >= 0)
					bytes.push(codepoint);
			}
			return new Uint8Array(bytes);
		},
		decoder: (buf:Uint8Array)=>{
			return [...buf].
				filter(byte=>byte < 0x80 && byte >= 0).
				map(byte=>String.fromCodePoint(byte)).
				join("");
		}

	},
	{name: "Codepage 437 (OEM/VGA)",          ...codepage_number(437)  },
	{name: "GB18030 (Simplified Chinese)",    ...codepage_number(54936)},
	{name: "ISO-8859-1 (West Europe)",        ...codepage_number(28591)},
	{name: "ISO-8859-2 (East Europe)",        ...codepage_number(28592)},
	{name: "ISO-8859-3 (Maltese)",            ...codepage_number(28593)},
	{name: "ISO-8859-4 (Scandinavia)",        ...codepage_number(28594)},
	{name: "ISO-8859-5 (Cyrillic)",           ...codepage_number(28595)},
	{name: "ISO-8859-6 (Arabic)",             ...codepage_number(28596)},
	{name: "ISO-8859-7 (Greek)",              ...codepage_number(28597)},
	{name: "ISO-8859-8 (Hebrew)",             ...codepage_number(28598)},
	{name: "ISO-8859-9 (Turkish)",            ...codepage_number(28599)},
	{name: "ISO-8859-10 (Nordic)",            ...codepage_number(28600)},
	{name: "ISO-8859-11 (Thai)",              ...codepage_number(28601)},
	{name: "ISO-8859-13 (Baltic)",            ...codepage_number(28603)},
	{name: "ISO-8859-14 (Celtic)",            ...codepage_number(28604)},
	{name: "ISO-8859-15 (West Europe)",       ...codepage_number(28605)},
	{name: "ISO-8859-16 (East Europe)",       ...codepage_number(28606)},
	{name: "KOI-8-R",                         ...codepage_number(20866)},
	{name: "KOI-8-U",                         ...codepage_number(21866)},
	{name: "Shift JIS (Japanese)",            ...codepage_number(932)  },
	{
		name: "Sign Extend (Common Bugged Implementation)",
		encoder: (str:string)=>{
			let bytes=[] as number[];
			for (let i=0; i<str.length; i++){
				let code=str.charCodeAt(i) % 256;
				if (code < -128) code+=256;
				if (code > 128) code-=256;
				if (code < 0) code+=65536;
				bytes.push(code);
			}
			return new Uint8Array(bytes);
		},
		decoder: (buf:Uint8Array)=>{
			let codepoints=[] as string[];
			for (let i=0; i<buf.length; i++){
				let byte=buf[i]%256;
				if (byte < -128) byte+=256;
				if (byte > 128) byte-=256;
				if (byte < 0) byte+=65536;
				codepoints.push(String.fromCodePoint(byte));
			}
			return codepoints.join("");
		}
	},
	{name: "UTF-16",                          ...codepage_number(1200) },
	{name: "Wansung (Korean)",                ...codepage_number(20949)},
	{name: "Windows 1250 (East Europe)",      ...codepage_number(1250) },
	{name: "Windows 1251 (Cyrillic)",         ...codepage_number(1251) },
	{name: "Windows 1252 (West Europe)",      ...codepage_number(1252) },
	{name: "Windows 1253 (Greek)",            ...codepage_number(1253) },
	{name: "Windows 1254 (Turkish)",          ...codepage_number(1250) },
	{name: "Windows 1255 (Hebrew)",           ...codepage_number(1255) },
	{name: "Windows 1256 (Arabic)",           ...codepage_number(1256) },
	{name: "Windows 1257 (Baltic)",           ...codepage_number(1257) },
	{name: "Windows 1258 (Vietnamese)",       ...codepage_number(1258) },
	{name: "EBCDIC \u{1F92E}",                ...codepage_number(37)   }
];
const bf_initial_state={
	tape_head: 16,
	tape: new Array(32).fill(0n) as bigint[],
	program : "",
	ip : 0,
	output: [] as number[],
	stack : [] as number[],
	input_buffer: [] as number[],
	run_til_tilde : false,
	to_break: false
};
const plaintext_mode=0;
const plaintext_with_substitution_mode=1;
const ansi_mode=2;
const html_mode=3;
const bitmap_mode=4;
const truecolor_bitmap_mode=5;

const eight_bit_mode=0;
const sixteen_bit_mode=1;
const bigint_bit_mode=2;

let bfstate = $state({...bf_initial_state});
$inspect(bfstate);
let current_encoding_index = $state(0);
let input_text=$state("");
let output_mode=$state(0);
let cell_size_index=$state(0);
let speed=$state(256);
let output_canvas : HTMLCanvasElement | undefined=$state(undefined);
let file : FileList | undefined = $state(undefined);
$inspect(output_canvas);
$effect(()=>{
	if (output_canvas != undefined){
		/*It needs to be in a promise so
		 *svelte won't redraw whenever bfstate changes*/

		new Promise(draw);
	}
});

function draw(){
	if (
		output_canvas!=undefined &&
		bfstate.output.length > 0
	){
		let ctx = output_canvas.getContext("2d");
		let image_data_arr = [] as number[];
		if (ctx){
			if (output_mode == truecolor_bitmap_mode){
				if (cell_size_index == bigint_bit_mode){
					image_data_arr=bfstate.output.flatMap(a=>{
						let buf=Number(a)%4294967296;
						return [
							(buf&0xff000000)>>24,
							(buf&0xff0000)>>16,
							(buf&0xff00)>>8,
							buf&0xff
						];
					})
				} else {
					image_data_arr=bfstate.output;
				}
			} else {
				image_data_arr=bfstate.output.flatMap(
					(bits)=>{
						let arr=[];
						let bitness=(cell_size_index==sixteen_bit_mode)?16:8;
						let mask=1<<(bitness-1);
						for (let i=0; i<bitness; i++){
							if (bits&mask)
								arr.push(255);
							else 
								arr.push(0);
							mask>>=1;
						}
						return arr.flatMap(bit=>[bit,bit,bit,255]);
						
					}
				)
			}
			
			let padding=new Array((320*4)-Math.floor(image_data_arr.length%(320*4))).fill(0);
			let image_data_raw=new Uint8ClampedArray([...image_data_arr,...padding]);
			console.log(`draw ${image_data_raw}`);
			let image_data = new ImageData(
				image_data_raw,
				320,image_data_raw.length/(320*4)
			);
			let y_offset=0;
			if (image_data_raw.length > 320*240*4){
				y_offset=image_data_raw.length/(320*4)-240;
			}
			window.createImageBitmap(
				image_data, 0,y_offset,320,240	
			).then(image_bitmap=>{
				ctx.drawImage(image_bitmap,0,0);
			})
		}
	}
}
function nearby_cells() : bigint[] {
	const head_coarse=Math.floor(bfstate.tape_head/16)*16;
	return bfstate.tape.slice(head_coarse, head_coarse+15);
}
function run() {
	if (bfstate.to_break) {
		bfstate.to_break=false;
	} else {
		let interval=speed;
		do {
			step();
			interval--;
		} while(interval>0 && !bfstate.to_break);
		if (output_mode==bitmap_mode || output_mode==truecolor_bitmap_mode)
			draw();
		if (!bfstate.to_break){
			setTimeout(run,0);
		}
	}
}
function step() : boolean {
	const command=bfstate.program.charAt(bfstate.ip);
	let did_input_on_empty_buffer=false;
	switch (String(command)){
		case '+': {
			bfstate.tape[bfstate.tape_head]++;
			const max=(cell_size_index==sixteen_bit_mode)?65536n:256n;
			if (cell_size_index!=bigint_bit_mode && bfstate.tape[bfstate.tape_head]>=max){
				bfstate.tape[bfstate.tape_head]=0n;
			}
		}
		break;
		case '-': {
			bfstate.tape[bfstate.tape_head]--;
			if (cell_size_index!=bigint_bit_mode && bfstate.tape[bfstate.tape_head] <= -1n){
				const max=(cell_size_index==sixteen_bit_mode)?65535n:255n;
				bfstate.tape[bfstate.tape_head]=max;
			}
			
		}
		break;
		case "<": {
			bfstate.tape_head--;
			if (bfstate.tape_head<0){
				const len=bfstate.tape.length;
				bfstate.tape_head=len*3-1;
				const empty=new Array(len*3).fill(0n);
				bfstate.tape=empty.concat(bfstate.tape);
			}
		}
		break;
		case ">": {
			bfstate.tape_head++;
			if (bfstate.tape_head>=bfstate.tape.length){
				const len=bfstate.tape.length;
				const empty=new Array(len*3).fill(0n);
				bfstate.tape=bfstate.tape.concat(empty);
			}
		}
		break;
		case ".": {
			bfstate.output.push(Number(bfstate.tape[bfstate.tape_head]));
		}
		break;
		case ",": {
			if (bfstate.input_buffer.length==0){
				if (input_text.length == 0){
					bfstate.to_break=true;
					did_input_on_empty_buffer=true;
				} else {
					if (cell_size_index == sixteen_bit_mode) {
						bfstate.input_buffer=[input_text.charCodeAt(0)];
						input_text=input_text.slice(1);
					} else {
						const codepoint=input_text.codePointAt(0)!;
						const encoder=encodings[current_encoding_index].encoder;
						const bytes=encoder(String.fromCodePoint(codepoint));
						bfstate.input_buffer=[...(bytes as number[])];
						if (codepoint>=0x10000) {
							input_text=input_text.slice(2);
						} else {
							input_text=input_text.slice(1);
						}
					}
				}

			} 
			if (!did_input_on_empty_buffer){
				bfstate.tape[bfstate.tape_head]=BigInt(bfstate.input_buffer[0]);
				bfstate.input_buffer=bfstate.input_buffer.slice(1);
			}
				

		}
		break;
		case "[": {
			if (bfstate.tape[bfstate.tape_head] == 0n) {
				bfstate.ip++;
				let bracket_count=1;
				while(bracket_count>0){
					if (bfstate.program[bfstate.ip] == "["){
						bracket_count++;
					} else if (bfstate.program[bfstate.ip] == "]"){
						bracket_count--;
					}
					bfstate.ip++;
				}
			} else {
				bfstate.stack.push(bfstate.ip);
			}
			
		}
		break;
		case "]": {
			if (bfstate.tape[bfstate.tape_head] == 0n) {
				bfstate.stack.pop()
			} else {
				bfstate.ip=bfstate.stack.at(-1)!;
			}
				
		}
		break;
		case '~': {
			bfstate.to_break=bfstate.run_til_tilde;
		}
		break;
	}
	if (!did_input_on_empty_buffer && bfstate.ip < bfstate.program.length){
		bfstate.ip++;
	} else {
		bfstate.to_break=true;
	}
	return bfstate.to_break;
}
function reset(){
	bfstate={...bf_initial_state};
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
			progstring=progstring.substring(0,i) +
				"\u{a0}" + progstring.substring(i+1);
		}
	}
	if (bracket_count != 0)
		isvalid=false;
	file=undefined;
	
	if (progstring != "" && isvalid)
		bfstate.program = progstring;
	if (output_mode == bitmap_mode || output_mode == truecolor_bitmap_mode){
		let ctx=output_canvas!.getContext("2d");
		if (ctx){
			ctx.rect(0,0,320,240);
			ctx.fillStyle="black";
			ctx.fill();
		}
	}
		
}
function getProgramAsString() : string{
	if (bfstate.program == ""){
		return "\u{a0}";
	} else {
		let program_chunked=[]
		for (let i=0; i<bfstate.program.length; i+=80){
			program_chunked.push(bfstate.program.slice(i,i+79))
		}
		return program_chunked.join("\n");
	}
}


function positionProgramCursor() : string {
	const row=Math.floor(bfstate.ip/80);
	return `translate(${bfstate.ip%80}ch,${row*2}em)`;
}
function get_output(): string{
	const decoder=encodings[current_encoding_index].decoder;
	if (cell_size_index == sixteen_bit_mode){
		return bfstate.output.map(a=>String.fromCharCode(a)).join("");
	} else {
		return decoder(Uint8Array.from(bfstate.output));
	}

}
function get_output_substitution(): string {
	let s=get_output();
	for (let i=0; i<s.length;i++) {
		let c=s.codePointAt(i)!;
		if (c==0x7f){
			s=s.substring(0,i) + "\u{2421}" + s.substring(i+1);
		} else if (c<0x20){
			let subc=String.fromCodePoint(c+0x2400);
			s=s.substring(0,i) + subc + s.substring(i+1);
		}
	}
	return s;
}
function getANSI() : string{
	let s=get_output();
	s=s.replaceAll(" ", "\u{a0}").
		replaceAll("&", "&amp;").
		replaceAll("<","&lt;").
		replaceAll(">","&gt;").
		replaceAll("\n","<br/>");
	return ansiHTML.default(s);
}
</script>


<main>
<div class="main">
	<div class="codebox-area">
		<textarea id="codebox" class="codebox"></textarea>
		<div class="button-area">
			<button onclick={()=>{
				bfstate.run_til_tilde=false;
				run()
			}}>run</button>
			<button onclick={()=>{
				bfstate.run_til_tilde=true;
				run()
			}}>run til ~</button>
			<button onclick={step}>step</button>
			<button onclick={reset}>reset</button>
			<button onclick={()=>bfstate.to_break=true}>stop</button>
		</div>
		<textarea class="input_box"
			bind:value={
			()=>input_text,
			(value)=>{input_text=value}
			}
		></textarea>
		<input bind:files={file} type="file"
			onchange={load_file}
		>
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
		style:transform={`translateX(${(bfstate.tape_head%16)*3+1.5}em)`}
		>^</div>
	</div>

	<div class="output-area">
		<div class="output">
			{#if output_mode==plaintext_mode}
{get_output()}
			{:else if output_mode==plaintext_with_substitution_mode}
{get_output_substitution()}
			{:else if output_mode==ansi_mode}
			{@html getANSI()}
			{:else if output_mode==html_mode}
			{@html get_output()}
			{:else if output_mode==bitmap_mode || output_mode==truecolor_bitmap_mode}
			<canvas
				bind:this={output_canvas}
				class="output-canvas"
				width=320 height=240
			></canvas>
			{/if}
		</div>
		
		<select class="encoding-selector"
			onchange={e=>current_encoding_index=(e.target! as HTMLSelectElement).selectedIndex}>
			{#each encodings as encoding}
			<option>{encoding.name}</option>
			{/each}
		</select>
		<select onchange={e=>output_mode=(e.target! as HTMLSelectElement).selectedIndex}>
			<option selected>Plaintext</option>
			<option>Plaintext w/ Ascii Escape Substitution</option>
			<option>ECMA-48 (ANSI)</option>
			<option>HTML</option>
			<option>Bitmap (1-bpp)</option>
			<option>Bitmap (RGBA)</option>
		</select>
		<select onchange={e=>{
			cell_size_index=(e.target! as HTMLSelectElement).selectedIndex;
		}}>
			<option selected>8-bit</option>
			<option>16-bit (UTF-16 Only)</option>
			<option>Bigint</option>
		</select>
		Speed:
		<input
		type="number"
		value=256
		onchange={e=>speed=Number.parseFloat((e.target! as HTMLInputElement).value)}
		>
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
	white-space: pre-line;
	word-break:break-word;
	
}
.output-canvas {
	width: 320px;
	height: 240px;
	background-color: black;
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
	min-height: 10em;
	min-width: 20em;
	width: auto;
	height: auto;
	background-color: lightgray;
	font-family: monospace;
	display:flex;
	flex-direction: column;
}
.cursor{ 
	position:absolute;
	top:2em;
	font-size:1em;
}
.program{
	width: 80ch;
	line-height: 2em;
}
.program-area{ 
	font-size:1em;
	margin-left: 0.5em;
	margin-right: 0.5em;
}
.cell{
	font-size:1em;
	width:3em;
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