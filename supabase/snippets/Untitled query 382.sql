-- 1. Create the public profiles table linked to Supabase Auth
CREATE TABLE public.profiles (
  -- This links the profile directly to the auth user uuid
  id UUID REFERENCES auth.users(id) ON DELETE CASCADE PRIMARY KEY,
  username TEXT UNIQUE NOT NULL,
  chips INT DEFAULT 1000 NOT NULL, -- Perfect for a poker/card app!
  created_at TIMESTAMP WITH TIME ZONE DEFAULT TIMEZONE('utc'::text, NOW()) NOT NULL
);

-- 2. Create a secure function that handles the new user insertion
CREATE OR REPLACE FUNCTION public.handle_new_user()
RETURNS TRIGGER AS $$
BEGIN
  INSERT INTO public.profiles (id, username, chips)
  VALUES (
    new.id,
    -- This pulls the 'display_name' out of your React app's options.data object
    COALESCE(new.raw_user_meta_data->>'display_name', 'Anonymous_Player'),
    1000
  );
  RETURN NEW;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- 3. Set up the trigger to fire instantly after a user signs up
CREATE OR REPLACE TRIGGER on_auth_user_created
  AFTER INSERT ON auth.users
  FOR EACH ROW EXECUTE FUNCTION public.handle_new_user();