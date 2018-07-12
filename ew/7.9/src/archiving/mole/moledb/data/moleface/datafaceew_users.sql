
-- Encrypt password by MySQL function PASSWORD();

DELETE FROM datafaceew_users WHERE usr_name in ('user', 'adminUser');

INSERT INTO `datafaceew_users` (`usr_name`,`pwd`,`Role`) 
VALUES ('user',       '*A1D2142CEE6124B304C4FBFC700E0990AD95C0F8',  'READ ONLY'),
       ('adminUser',  '*032720965928C5D005598917CD63BEFEE81A9351',  'ADMIN');

