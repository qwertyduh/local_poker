import { createClient } from '@supabase/supabase-js'

// These lines look for the values on your "Secret Sticky Note" (.env)
const supabaseUrl = import.meta.env.VITE_SUPABASE_URL
const supabaseAnonKey = import.meta.env.VITE_SUPABASE_ANON_KEY

// This creates the actual "connection" object
export const supabase = createClient(supabaseUrl, supabaseAnonKey)
