import { SanctifyPveOfflineClientInstance, SanctifyPveOfflineClientLoadService } from "@sanctify/pve-offline-wasm-wrapper";
import Head from "next/head";
import Script from "next/script";
import React, { useEffect, useRef, useState } from "react";
import SanctifyGameApi from '../services/pve-offline-load-service';

type GameplayElementProps = {
  api: SanctifyPveOfflineClientLoadService,
  setGameClient: React.Dispatch<React.SetStateAction<SanctifyPveOfflineClientInstance|null>>,
};

function GameplayElement({api, setGameClient}: GameplayElementProps) {
  const parentDivRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    let __client: SanctifyPveOfflineClientInstance|null = null;

    let stupidHackIntervalHandle = setInterval(() => {
      if (parentDivRef.current != null) {
        const canvas = api.getGameCanvas();
        parentDivRef.current.appendChild(canvas);

        api.getGameClient().then(client => {
          console.log('Creating the game client!');
          setGameClient(client);
          client.start();
          __client = client;
        });
        clearInterval(stupidHackIntervalHandle);
      }
    }, 100);

    return () => {
      console.log('Destroying the game client!');
      __client?.destroy();
    };
  }, []);

  return <div ref={parentDivRef} style={{
    width: '100%',
    height: '100%',
    display: 'block',
  }} />;
}

export default function QuickPlay() {
  const api = SanctifyGameApi('');

  const [scriptLoaded, setScriptLoaded] = useState(false);
  const [gameClient, setGameClient] = useState<SanctifyPveOfflineClientInstance|null>(null);

  return (
    <div>
      <Head>
        <title>Loading...</title>
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <Script src={api.gameScriptUrl()} strategy="afterInteractive" onLoad={()=>setScriptLoaded(true)} />
      {!scriptLoaded && <div>Loading script...</div>}
      {!gameClient && <div>Loading game...</div>}
      {scriptLoaded && <GameplayElement api={api} setGameClient={setGameClient} />}
    </div>
  )
}
