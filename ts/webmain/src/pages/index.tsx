import Head from 'next/head';

import BaseContainer from '../components/base-container';

import styles from '../styles/home.module.scss';

export default function Home() {
  return (
    <div>
      <Head>
        <title>Sanctify Game</title>
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <BaseContainer>
        <main>
          <div className={`game-quote ${styles['game-title']}`}>Sanctify</div>
        </main>
      </BaseContainer>
    </div>
  );
}
