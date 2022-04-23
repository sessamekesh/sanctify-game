/**
 * Wrapper around the WebAssembly module for the SanctifyPveOfflineClient CMake target in cpp/
 * 
 * It's a wee gross, but I've had pretty good success with this pattern in the past. If there's a
 *  weird bug in the WASM interface, it's probably because of something in here which is nice.
 * 
 * Notice: Usage of any of the classes in here requires that the WASM is already loaded through the
 *  Emscripten generated shim script!
 */

/// <reference types="@webgpu/types" />

import { PveOfflineClientConfig } from './pve_offline_client_config';

// Emscripten object (see set_wasm_target_properties call in CMakeLists.txt)
declare const SanctifyPveOfflineClient: any;

export class SanctifyPveOfflineClientInstance {
  private hdl: number | undefined;

  /** DO NOT CALL THIS CONSTRUCTOR DIRECTLY */
  /** I WILL COME TO YOUR HOME AND SMACK YOU RIGHT IN THE FACE */
  /** USE `SanctifyPveOfflineClientBridge.createClient() INSTEAD */
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

  runTasksFor(s: number) {
    this.wasmObj['run_tasks_for'](s);
  }

  destroy() {
    this.pause();
    this.wasmObj['delete']();
  }

  shouldQuit(): boolean {
    return this.wasmObj['should_quit']();
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

      const runTasksDuration = now + 16.666 - performance.now();
      this.runTasksFor(runTasksDuration / 1000);

      if (!this.shouldQuit()) {
        this.hdl = requestAnimationFrame(frame);
      }
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

export class SanctifyPveOfflineClientBridge {
  private constructor(private wasmInstance: any, private canvas: HTMLCanvasElement) { }

  static async Create(canvas: HTMLCanvasElement, adapter: GPUAdapter, webGpuDevice: GPUDevice): Promise<SanctifyPveOfflineClientBridge> {
    return new Promise((resolve, reject) => {
      SanctifyPveOfflineClient({ canvas })
        .then((inst: any) => {
          inst['preinitializedWebGPUDevice'] = webGpuDevice;
          const context = canvas.getContext('webgpu');
          inst['evil_hack_set_preferred_wgpu_texture_cb'](() => {
            // 0 -> wgpu::TextureFormat::BGRA8Unorm;
            // 1 -> wgpu::TextureFormat::RGBA8Unorm;
            const format = context?.getPreferredFormat(adapter) ?? '--none-read--';
            switch (format) {
              case 'rgba8unorm':
              case '--none-read--':
              default:
                return 1;
              case 'bgra8unorm':
                return 0;
            }
          });
          resolve(new SanctifyPveOfflineClientBridge(inst, canvas));
        })
        .catch((e: any) => reject(e));
    });
  }

  /** Create a new client instance */
  createClient(config?: PveOfflineClientConfig): SanctifyPveOfflineClientInstance {
    if (config == null) {
      config = PveOfflineClientConfig.fromJSON({});
      config.renderSettings = {swapChainResizeLatency: 0.5};
    }

    const configBin = PveOfflineClientConfig.encode(config).finish();

    const wasmInstance = this.wasmInstance['SanctifyPveOfflineClient']['Create'](configBin);
    return new SanctifyPveOfflineClientInstance(wasmInstance, this.wasmInstance, this.canvas);
  }
}
