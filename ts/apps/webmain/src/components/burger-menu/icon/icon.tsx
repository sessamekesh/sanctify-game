import styles from './style.module.scss';

type Props = {
  onOpen: () => void;
};

export default function BurgerMenuIcon({onOpen}: Props) {
  return (
    <div className={styles['icon-holder']} onClick={onOpen}>
      <span className={styles['bar']}></span>
      <span className={styles['bar']}></span>
      <span className={styles['bar']}></span>
    </div>
  );
}
