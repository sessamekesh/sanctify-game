import {SanctifyGameClientInstance, SanctifyClientWasmBridge} from './sanctify-wasm';

export const SUPPORTS_MULTITHREADING = typeof SharedArrayBuffer === 'function';

export class SanctifyGameClientLoadService {
  private scriptElement: HTMLScriptElement|null = null;
  private gameCanvas: HTMLCanvasElement|null = null;
  private gameClient: SanctifyGameClientInstance|null = null;

  constructor(private readonly baseUrl: string) {}

  async loadGameScript(): Promise<void> {
    if (this.scriptElement) {
      return;
    }

    const src = SUPPORTS_MULTITHREADING
      ? `${this.baseUrl}/wasm_mt/sanctify-game-client.js`
      : `${this.baseUrl}/wasm_st/sanctify-game-client.js`;
    
    return new Promise((resolve, reject) => {
      const scriptElement = document.createElement('script');
      scriptElement.onload = () => {
        this.scriptElement = scriptElement;
        resolve();
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
    }

    return this.gameCanvas;
  }

  async getGameClient(): Promise<SanctifyGameClientInstance> {
    if (!this.gameClient) {
      if (!navigator.gpu) {
        throw new Error('WebGPU is not supported');
      }

      const canvas = this.getGameCanvas();
      canvas.width = canvas.clientHeight * devicePixelRatio;
      canvas.height = canvas.clientWidth * devicePixelRatio;

      const [_empty, adapter] = await Promise.all([
        this.loadGameScript(),
        navigator.gpu.requestAdapter()
      ]);

      if (!adapter) {
        throw new Error('Could not get WebGPU adapter');
      }

      const device = await adapter.requestDevice();

      const wasmModule = await SanctifyClientWasmBridge.Create(canvas, device);
      this.gameClient = wasmModule.createClient();
    }

    return this.gameClient;
  }
}
