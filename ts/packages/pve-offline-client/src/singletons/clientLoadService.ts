import { SanctifyPveOfflineClientLoadService } from "@sanctify/pve-offline-wasm";
import { useState } from "react";
import { singletonHook } from "react-singleton-hook";

let __setBaseUrl: (baseUrl: string) => void = (dummy: string) => { };

let url = '';
let client = new SanctifyPveOfflineClientLoadService('');

export const pveOfflineLoadService = singletonHook(client, () => {
  const [baseUrl, setBaseUrl] = useState('');
  __setBaseUrl = setBaseUrl;
  if (baseUrl === url) {
    return client;
  }

  url = baseUrl;
  client = new SanctifyPveOfflineClientLoadService(baseUrl);
  return client;
});

export const setBaseUrl = __setBaseUrl;
