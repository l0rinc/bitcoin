Updated settings
----------------

- Proxy settings now respect source precedence: command-line options override `settings.json`, which overrides `bitcoin.conf`.
  A general or onion-specific proxy address from a higher-priority source also overrides lower-priority `-onion` settings, including `-onion=0`.
  Disabling the general proxy preserves an explicit `-onion` setting.
