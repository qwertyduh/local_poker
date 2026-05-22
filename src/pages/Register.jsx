import { useState } from 'react'
import { supabase } from '../lib/supabaseClient'
import { Link, useNavigate } from 'react-router-dom'

export default function Register() {
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [username, setUsername] = useState('')
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')
  const navigate = useNavigate()

  const handleSignUp = async (e) => {
    e.preventDefault()
    setLoading(true)
    setError('')

    // 1. Sign up the user in Supabase Auth
    const { data, error: authError } = await supabase.auth.signUp({
      email,
      password,
      options: {
        // We save the username in 'metadata' so the DB trigger can grab it
        data: { username: username }
      }
    })

    if (authError) {
      setError(authError.message)
      setLoading(false)
      return
    }

    alert('Check your email for a confirmation link!')
    navigate('/') // Send them back to login after success
  }

  return (
    <main className="container">
      <header className="logo">
        <div className="logo-mark">
          <div className="logo-icon">♠</div>
          <h1 className="logo-text">FELT</h1>
        </div>
        <p className="logo-tagline">NEW PLAYER REGISTRATION</p>
      </header>

      <section className="card">
        <h2 className="card-title">Create Identity</h2>
        
        {error && <div className="error-msg show">{error}</div>}

        <form onSubmit={handleSignUp}>
          <div className="field">
            <label>Email Address</label>
            <input type="email" value={email} onChange={(e) => setEmail(e.target.value)} required 
              style={{ width: '100%', background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', color: 'white' }} 
            />
          </div>

          <div className="field">
            <label>Password</label>
            <input type="password" value={password} onChange={(e) => setPassword(e.target.value)} required 
               style={{ width: '100%', background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', color: 'white' }}
            />
          </div>

          <div className="field">
            <label>Player Handle (Username)</label>
            <input type="text" value={username} onChange={(e) => setUsername(e.target.value)} required 
               style={{ width: '100%', background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', color: 'white' }}
            />
          </div>

          <button type="submit" className="btn-primary" disabled={loading}>
            {loading ? 'Issuing Credentials...' : 'Register to Play →'}
          </button>
        </form>

        <div className="divider">ALREADY A MEMBER?</div>
        <Link to="/" className="btn-primary" style={{ display: 'block', textAlign: 'center', background: 'transparent', color: '#8a9ba8', border: '1px solid rgba(255,255,255,0.1)', textDecoration: 'none' }}>
          Back to Entrance
        </Link>
      </section>
    </main>
  )
}
