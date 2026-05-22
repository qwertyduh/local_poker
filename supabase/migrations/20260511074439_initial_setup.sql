create table public.profiles (
  -- Link to Supabase Auth
  id uuid references auth.users on delete cascade not null primary key,
  
  -- Identity
  username text unique not null,
  avatar_url text,
  
  -- Currency (Stored in cents/chips)
  balance bigint default 1000 check (balance >= 0),
  
  -- Stats
  total_wins integer default 0 check (total_wins >= 0),
  total_games_played integer default 0 check (total_games_played >= 0),
  xp integer default 0,
  
  -- Settings & Metadata
  preferences jsonb default '{}'::jsonb,
  last_seen_at timestamp with time zone default now(),
  created_at timestamp with time zone default now()
);

-- Enable RLS
alter table public.profiles enable row level security;

-- Policy: Users can only update their own profile
create policy "Users can update own profile" 
  on profiles for update 
  using ( auth.uid() = id );

-- Policy: Everyone can see usernames/stats
create policy "Profiles are public" 
  on profiles for select 
  using ( true );