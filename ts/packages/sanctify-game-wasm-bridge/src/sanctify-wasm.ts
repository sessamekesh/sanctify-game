/** 
 * Wrapper around the WebAssembly module declared in bind_web.cc. It's a wee gross, but I've had
 *  pretty good success with this pattern in the past. If there's a weird bug in the WASM interface,
 *  it's probably because of something in here which is nice.
 * 
 * Notice: Usage of any of the classes in here requires that the WASM is already loaded through the
 *  Emscripten generated shim script!
 */

 declare const SanctifyGameClient: any;

 export class SanctifyGameClientInstance {
   private hdl: number|undefined;
 
   /** DO NOT CALL THIS CONSTRUCTOR DIRECTLY */
   /** I WILL COME TO YOUR HOME AND SMACK YOU RIGHT IN THE FACE */
   /** USE `SanctifyClientWasmBridge.createClient() INSTEAD */
   constructor(private wasmObj: any, private wasmModule: any, private c: HTMLCanvasElement) {
   }
 
   update(dt: number) {
     this.wasmObj['update'](dt);
   }
 
   render() {
     this.c.width = this.c.clientWidth * devicePixelRatio;
     this.c.height = this.c.clientHeight * devicePixelRatio;
     this.wasmObj['render']();
   }
 
   runTasksFor(ms: number) {
     this.wasmObj['run_tasks_for'](ms);
   }
 
   destroy() {
     this.wasmObj['delete']();
   }
 
   start() {
     // This is a silly hack that is required because Emscripten resizes the canvas. You should
     //  prevent them from doing that.
     this.c.style.width = '100%';
     this.c.style.height = '100%';
 
     let lastFrame = performance.now();
     const frame = () => {
       const now = performance.now();
       const dt = (now - lastFrame) / 1000;
       lastFrame = now;
 
       this.update(dt);
       this.render();
 
       const runTasksDuration = now + 16 - performance.now();
       this.runTasksFor(runTasksDuration);
 
       this.hdl = requestAnimationFrame(frame);
     };
     this.hdl = requestAnimationFrame(frame);
   }
 
   pause() {
     if (this.hdl != null) {
       cancelAnimationFrame(this.hdl);
       this.hdl = undefined;
     }
   }
 }
 
 export class SanctifyClientWasmBridge {
   private constructor(private wasmInstance: any, private canvas: HTMLCanvasElement) {}
 
   static Create(canvas: HTMLCanvasElement, webGpuDevice: GPUDevice): Promise<SanctifyClientWasmBridge> {
     return new Promise((resolve, reject) => {
        SanctifyGameClient({canvas})
         .then((inst: any) => {
           inst['preinitializedWebGPUDevice'] = webGpuDevice;
           resolve(new SanctifyClientWasmBridge(inst, canvas));
         })
         .catch((e: any) => reject(e));
     });
   }
 
   /** Create a new Indigo Demos client instance */
   createClient(): SanctifyGameClientInstance {
     const wasmInstance = this.wasmInstance['SanctifyClientApp']['Create'](
       this.canvas.width, this.canvas.height);
 
     const tr = new SanctifyGameClientInstance(wasmInstance, this.wasmInstance, this.canvas);
     return tr;
   }
 }