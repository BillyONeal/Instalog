-- Files table. Entries will be whitelisted (or not) based on whether they have
-- an entry in this table.
--
-- Before being entered, paths are normalized into UPPERCASE according to
-- NTFS' rules, and common paths like %WINDOWS%, %USERPROFILE%, %PROGRAMFILES% etc.
-- are replaced.


CREATE TABLE Files (
    Path STRING NOT NULL PRIMARY KEY,  -- Path after normalization happens. See normalization spec above.
    WhitelistLevel INTEGER NOT NULL    -- Invisible unless the verbosity level is
                                       -- set to more than this number, [0-5)
) WITHOUT ROWID;

-- Only show explorer.exe when whitelisting is off.

INSERT INTO FILES (Path, WhitelistLevel) VALUES (
    "%WINDIR%\\EXPLORER.EXE",
    5
);

