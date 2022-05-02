import Link from 'next/link';
import React from 'react';
import styles from './base-header.module.scss';
import BurgerMenu from '../../burger-menu';

const BaseHeader = () => {
  return (
    <div className={styles['header-base']}>
      <BurgerMenu>
        <Link href="/">Home</Link>
        <h3>Account</h3>
        <Link href="/login">Login</Link>
        <h3>Game</h3>
        <Link href="/quickplay">Quick Play</Link>
      </BurgerMenu>
      <Link href="/login">Login/Register</Link>
    </div>
  );
};

export default BaseHeader;
