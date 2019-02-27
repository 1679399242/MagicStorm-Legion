DELETE FROM broadcast_text WHERE ID = 147279;
INSERT INTO broadcast_text (ID, Text) VALUES
(147279, "Right away, General!");
DELETE FROM broadcast_text_locale WHERE ID = 147279;
INSERT INTO broadcast_text_locale (ID, locale, Text_lang) VALUES
(147279, "ruRU", "Сейчас же, Генерал!");
