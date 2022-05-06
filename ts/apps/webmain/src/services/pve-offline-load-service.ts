import { ClientLoadService } from '@sanctify/pve-offline-wasm';

let loaded: ClientLoadService|null = null;

export default function PveOfflineLoadService(): ClientLoadService {
  if (!loaded) {
    loaded = new ClientLoadService();
  }
  return loaded;
}

export function unloadService() {
  if (loaded) {
    // TODO (sessamekesh): Unload the actual client here - destroy everything!
    return;
  }
}
