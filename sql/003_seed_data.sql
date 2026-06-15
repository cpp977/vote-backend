-- 003_seed_data.sql

-- Insert Categories
INSERT INTO categories (name) VALUES
('Food'),
('Mobility'),
('Lifestyle'),
('Technology'),
('Environment'),
('Work'),
('Health'),
('Finance'),
('Education'),
('Entertainment');

-- Insert Questions and Answer Options

-- 1. How many bananas do you eat per week? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How many bananas do you eat per week?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '0');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '3-5');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '6-10');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 10');

-- 2. Do you have an own car? (Mobility)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you have an own car?', (SELECT id FROM categories WHERE name = 'Mobility'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Yes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'No');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'I share one');

-- 3. How often do you eat fast food per month? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you eat fast food per month?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '3-5 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '6-10 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 10 times');

-- 4. What is your primary mode of transport? (Mobility)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your primary mode of transport?', (SELECT id FROM categories WHERE name = 'Mobility'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Car');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Public Transport');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Bicycle');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Walking');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Motorcycle/Scooter');

-- 5. How many hours do you sleep per night? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How many hours do you sleep per night?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than 5');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '5-6');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '7-8');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '9-10');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 10');

-- 6. How often do you use social media per day? (Technology)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you use social media per day?', (SELECT id FROM categories WHERE name = 'Technology'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than 30 minutes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '30-60 minutes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2 hours');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-4 hours');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 4 hours');

-- 7. Do you recycle regularly? (Environment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you recycle regularly?', (SELECT id FROM categories WHERE name = 'Environment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Always');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Often');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Sometimes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Rarely');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');

-- 8. How many cups of coffee do you drink per day? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How many cups of coffee do you drink per day?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '0');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-3');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '4-5');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 5');

-- 9. What is your employment status? (Work)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your employment status?', (SELECT id FROM categories WHERE name = 'Work'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Employed Full-time');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Employed Part-time');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Self-employed');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Unemployed');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Student');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Retired');

-- 10. How often do you exercise per week? (Health)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you exercise per week?', (SELECT id FROM categories WHERE name = 'Health'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '3-4 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '5-6 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Every day');

-- 11. What is your highest level of education? (Education)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your highest level of education?', (SELECT id FROM categories WHERE name = 'Education'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'High School');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Bachelor''s Degree');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Master''s Degree');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'PhD');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Other');

-- 12. How much do you spend on groceries per week? (Finance)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How much do you spend on groceries per week?', (SELECT id FROM categories WHERE name = 'Finance'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than $50');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '$50 - $100');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '$100 - $150');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '$150 - $200');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than $200');

-- 13. What is your favorite type of music? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of music?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Pop');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Rock');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Hip Hop');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Classical');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Jazz');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Electronic');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Country');

-- 14. How many hours do you spend watching TV per day? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How many hours do you spend watching TV per day?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than 1');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-4');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '4-6');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 6');

-- 15. Do you prefer cats or dogs? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you prefer cats or dogs?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cats');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Dogs');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Both equally');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Neither');

-- 16. How often do you travel by plane per year? (Mobility)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you travel by plane per year?', (SELECT id FROM categories WHERE name = 'Mobility'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '3-5 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '6-10 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 10 times');

-- 17. What is your favorite season? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite season?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Spring');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Summer');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Autumn');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Winter');

-- 18. How many books do you read per year? (Education)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How many books do you read per year?', (SELECT id FROM categories WHERE name = 'Education'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '0');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-5');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '6-10');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '11-20');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 20');

-- 19. Do you use a smartphone? (Technology)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you use a smartphone?', (SELECT id FROM categories WHERE name = 'Technology'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Yes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'No');

-- 20. How much water do you drink per day? (Health)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How much water do you drink per day?', (SELECT id FROM categories WHERE name = 'Health'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than 1 liter');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2 liters');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-3 liters');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 3 liters');

-- 21. What is your favorite type of cuisine? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of cuisine?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Italian');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Mexican');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Chinese');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Indian');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Japanese');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'American');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Mediterranean');

-- 22. Do you have a gym membership? (Health)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you have a gym membership?', (SELECT id FROM categories WHERE name = 'Health'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Yes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'No');

-- 23. How many hours do you spend on your phone per day? (Technology)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How many hours do you spend on your phone per day?', (SELECT id FROM categories WHERE name = 'Technology'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than 1');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-4');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '4-6');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 6');

-- 24. What is your favorite social media platform? (Technology)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite social media platform?', (SELECT id FROM categories WHERE name = 'Technology'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Facebook');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Instagram');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Twitter');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'TikTok');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'LinkedIn');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Snapchat');

-- 25. Do you prefer tea or coffee? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you prefer tea or coffee?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Tea');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Coffee');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Both equally');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Neither');

-- 26. How often do you eat out per month? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you eat out per month?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '3-5 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '6-10 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 10 times');

-- 27. What is your favorite type of movie? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of movie?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Action');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Comedy');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Drama');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Horror');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Sci-Fi');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Romance');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Documentary');

-- 28. Do you play video games? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you play video games?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Daily');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Weekly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Monthly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Rarely');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');

-- 29. What is your favorite type of pet? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of pet?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Dog');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cat');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Fish');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Bird');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Reptile');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Hamster/Guinea Pig');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'None');

-- 30. How often do you listen to podcasts? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you listen to podcasts?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Daily');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Weekly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Monthly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Rarely');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');

-- 31. What is your favorite type of weather? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of weather?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Sunny');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Rainy');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Snowy');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cloudy');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Windy');

-- 32. Do you use public transportation? (Mobility)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you use public transportation?', (SELECT id FROM categories WHERE name = 'Mobility'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Daily');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Weekly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Monthly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Rarely');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');

-- 33. How many hours do you spend on homework/studying per day? (Education)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How many hours do you spend on homework/studying per day?', (SELECT id FROM categories WHERE name = 'Education'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than 1');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-2');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-4');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '4-6');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 6');

-- 34. What is your favorite type of sport? (Health)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of sport?', (SELECT id FROM categories WHERE name = 'Health'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Soccer');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Basketball');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Tennis');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Swimming');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Running');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cycling');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Other');

-- 35. Do you prefer sweet or salty snacks? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you prefer sweet or salty snacks?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Sweet');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Salty');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Both equally');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Neither');

-- 36. How often do you go to the cinema per year? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you go to the cinema per year?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '1-3 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '4-6 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '7-12 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than 12 times');

-- 37. What is your favorite type of vacation? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of vacation?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Beach');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Mountain');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'City Break');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cruise');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Adventure');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Staycation');

-- 38. Do you use a streaming service? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you use a streaming service?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Yes, one');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Yes, multiple');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'No');

-- 39. How much do you spend on entertainment per month? (Finance)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How much do you spend on entertainment per month?', (SELECT id FROM categories WHERE name = 'Finance'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than $20');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '$20 - $50');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '$50 - $100');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '$100 - $200');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'More than $200');

-- 40. What is your favorite type of art? (Entertainment)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of art?', (SELECT id FROM categories WHERE name = 'Entertainment'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Painting');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Sculpture');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Photography');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Digital Art');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Music');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Dance');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Theater');

-- 41. Do you prefer to shop online or in-store? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you prefer to shop online or in-store?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Online');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'In-store');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Both equally');

-- 42. How often do you volunteer? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you volunteer?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Weekly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Monthly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Yearly');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Rarely');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Never');

-- 43. What is your favorite type of drink? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of drink?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Water');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Soda');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Juice');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Milk');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Beer');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Wine');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cocktails');

-- 44. Do you have a garden? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you have a garden?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Yes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'No');

-- 45. How often do you clean your house per week? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you clean your house per week?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Every day');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-3 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Once');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than once');

-- 46. What is your favorite type of fruit? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of fruit?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Apple');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Banana');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Orange');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Strawberry');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Grapes');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Watermelon');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Pineapple');

-- 47. Do you prefer to cook or order takeout? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you prefer to cook or order takeout?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cook');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Order takeout');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Both equally');

-- 48. How often do you visit your family per month? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('How often do you visit your family per month?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Every week');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), '2-3 times');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Once');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Less than once');

-- 49. What is your favorite type of dessert? (Food)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('What is your favorite type of dessert?', (SELECT id FROM categories WHERE name = 'Food'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cake');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Ice Cream');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Cookies');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Pie');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Candy');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Chocolate');

-- 50. Do you prefer to travel alone or with others? (Lifestyle)
INSERT INTO questions (text, category_id, language, min_age) VALUES ('Do you prefer to travel alone or with others?', (SELECT id FROM categories WHERE name = 'Lifestyle'), 'en', 0);
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'Alone');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'With partner');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'With friends');
INSERT INTO answer_options (question_id, text) VALUES (currval('questions_id_seq'), 'With family');
