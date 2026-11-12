import { createRoot } from 'react-dom/client';
import App from './App';
import './styles.css';
import './ui/observer-shell.css';

const root = document.getElementById('root');
if (!root) throw new Error('Missing React root');

createRoot(root).render(<App />);

