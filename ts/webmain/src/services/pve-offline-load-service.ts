import { SanctifyPveOfflineClientLoadService } from '@sanctify/pve-offline-wasm-wrapper';

const loaded: Map<string, SanctifyPveOfflineClientLoadService> = new Map();

export default function PveOfflineLoadService(baseUrl: string): SanctifyPveOfflineClientLoadService {
  if (loaded.has(baseUrl)) {
    return loaded.get(baseUrl)!;
  }

  const client = new SanctifyPveOfflineClientLoadService(baseUrl);
  loaded.set(baseUrl, client);
  return client;
}

export function unloadService(baseUrl: string) {
  const client = loaded.get(baseUrl);
  if (!client) {
    return;
  }

  // TODO (sessamekesh): Unload the actual client here - destroy everything!
  loaded.delete(baseUrl);
}
