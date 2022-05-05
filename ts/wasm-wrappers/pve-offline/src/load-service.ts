import { SanctifyPveOfflineClientBridge, SanctifyPveOfflineClientInstance } from './wasm-bridge';

export const SUPPORTS_MULTITHREADING = typeof SharedArrayBuffer === 'function';

export class SanctifyPveOfflineClientLoadService {
  private scriptElementPromise: Promise<HTMLScriptElement>|null = null;
  private gameCanvas: HTMLCanvasElement|null = null;
  private gameClientPromise: Promise<SanctifyPveOfflineClientInstance>|null = null;

  constructor(private readonly baseUrl: string) {}

  gameScriptUrl(): string {
    return SUPPORTS_MULTITHREADING
      ? `${this.baseUrl}/wasm_mt/sanctify-pve-offline-client.js`
      : `${this.baseUrl}/wasm_st/sanctify-pve-offline-client.js`;
  }

  async loadGameScript(): Promise<void> {
    if (this.scriptElementPromise) {
      return;
    }

    const src = this.gameScriptUrl();
    
    this.scriptElementPromise = new Promise((resolve, reject) => {
      const scriptElement = document.createElement('script');
      scriptElement.onload = () => {
        resolve(scriptElement);
      };
      scriptElement.onerror = () => {
        document.body.removeChild(scriptElement);
        reject();
      };
      scriptElement.src = src;
      document.body.appendChild(scriptElement);
    });
  }

  /**
   * Get the canvas element used for the game
   * 
   * Notice! This doesn't handle sizing - callers must handle sizing/placement on their own!
   */
  getGameCanvas() {
    if (this.gameCanvas === null) {
      this.gameCanvas = document.createElement('canvas');

      this.gameCanvas.id = 'app_canvas';
      this.gameCanvas.style.width = '100%';
      this.gameCanvas.style.height = '100%';
      this.gameCanvas.style.display = 'block';
    }

    return this.gameCanvas;
  }

  async getGameClient(): Promise<SanctifyPveOfflineClientInstance> {
    if (!this.gameClientPromise) {
      if (!navigator.gpu) {
        throw new Error('WebGPU is not supported');
      }

      const canvas = this.getGameCanvas();
      canvas.width = canvas.clientHeight * devicePixelRatio;
      canvas.height = canvas.clientWidth * devicePixelRatio;

      this.gameClientPromise = (async () => {
        const [_empty, adapter] = await Promise.all([
          this.loadGameScript(),
          navigator.gpu.requestAdapter()
        ]);
  
        if (!adapter) {
          throw new Error('Could not get WebGPU adapter');
        }
  
        const device = await adapter.requestDevice();
  
        const wasmModule = await SanctifyPveOfflineClientBridge.Create(canvas, adapter, device);
        return wasmModule.createClient();
      })();
    }

    return this.gameClientPromise;
  }
}
