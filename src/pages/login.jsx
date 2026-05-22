import { useState } from 'react'
import { supabase } from '../lib/supabaseClient' // Import the 'phone' we built
import { useNavigate } from 'react-router-dom'
import '../index.css'

function App() {
  const navigate = useNavigate()
  const [username, setUsername] = useState('')
  const [error, setError] = useState(false)
  const [loading, setLoading] = useState(false)

  const handleJoin = async (e) => {
    e.preventDefault()
    
    // Basic validation
    if (!username.trim()) {
      setError(true)
      return
    }

    setError(false)
    setLoading(true)

    try {
      // 1. Search the 'profiles' table for this username
      const { data, error: dbError } = await supabase
        .from('profiles')
        .select('username')
        .eq('username', username)
        .single()

      if (dbError && dbError.code !== 'PGRST116') {
        // PGRST116 is the code for "no rows found", which we handle below
        throw dbError
      }

      if (data) {
        // CASE: User Exists
        alert(`Welcome back, ${data.username}! Fetching your chips...`)
        // Future: Redirect to Lobby
      } else {
        // CASE: User Not Found
        alert(`Account not found. Sending you to the registration desk...`)
        navigate('/Register')
      }
    } catch (err) {
      console.error('Error searching user:', err)
      alert('The pit boss is busy (Database Error). Try again later.')
    } finally {
      setLoading(false)
    }
  }

  return (
    <main className="container">
      <header className="logo">
        <div className="logo-mark">
          <div className="logo-icon">♠</div>
          <h1 className="logo-text">FELT</h1>
        </div>
        <p className="logo-tagline">PRIVATE TABLE • NO RAKE • ALL FRIENDS</p>
      </header>

      <section className="card">
        <h2 className="card-title">Join Table</h2>
        
        <div className={`error-msg ${error ? 'show' : ''}`}>
          Please enter a valid handle to play.
        </div>

        <form onSubmit={handleJoin}>
          <div className="field">
            <label htmlFor="username">Player Handle</label>
            <input 
              type="text" 
              id="username"
              placeholder="e.g. PokerKing_01"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              disabled={loading}
            />
          </div>

          <button type="submit" className="btn-primary" disabled={loading}>
            {loading ? 'Checking Ledger...' : 'Take a Seat →'}
          </button>
        </form>
      </section>

      <div className="suit-row">
        <span className="suit">♠</span>
        <span className="suit">♥</span>
        <span className="suit">♦</span>
        <span className="suit">♣</span>
      </div>
    </main>
  )
}

export default App