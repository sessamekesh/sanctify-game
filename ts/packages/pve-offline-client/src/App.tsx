import { SanctifyPveOfflineClientInstance } from '@sanctify/pve-offline-wasm';
import React, { useEffect, useRef } from 'react';
import styles from './App.module.scss';
import { pveOfflineLoadService } from './singletons/clientLoadService';

type Params = {
  baseUrl: string,
};

// This is an evil hack to try to keep only one version of the client running about.
// I don't know how "useEffect" works, so it ends up being called twice somehow.
let __client: SanctifyPveOfflineClientInstance|null = null;

function App({ baseUrl }: Params) {
  const mountRef = useRef<HTMLDivElement>(null);
  const loadService = pveOfflineLoadService();
  const [isLoading, setIsLoading] = React.useState<boolean>(true);
  const [wgpuError, setWgpuError] = React.useState<boolean>(false);
  const [hasUnexpectedError, setHasUnexpectedError] = React.useState(false);

  useEffect(() => {
    if (mountRef.current == null) {
      setIsLoading(false);
      setHasUnexpectedError(true);
      return;
    }

    const canvas = loadService.getGameCanvas();
    canvas.className = styles['game-canvas'];

    mountRef.current.appendChild(canvas);

    loadService.getGameClient()
      .then((c) => {
        if (__client === c) {
          return;
        }
        if (__client) {
          __client.destroy();
        }
        __client = c;
        __client.start();
      })
      .catch((e) => {
        setWgpuError(e);
        setIsLoading(false);
      });
    
    return function cleanup() {
      if (__client) {
        __client.destroy();
      }
      canvas.remove();
    };
  }, [loadService]);

  if (isLoading) {
    return <div>Loading...<div ref={mountRef} /></div>;
  }

  if (wgpuError) {
    return <div>WebGPU could not be initialized</div>;
  }

  if (hasUnexpectedError) {
    return <div>An unexpected error occurred</div>;
  }

  return (
    <div ref={mountRef} className={styles['fullscreen-wrapper']} />
  );
}

export default App;
