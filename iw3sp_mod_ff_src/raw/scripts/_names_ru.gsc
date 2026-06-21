init()
{
}

GetTranslatedName( name )
{
    // American names
    if (name == "Abrahamsson") return "Абрахамссон";
    else if (name == "Alavi") return "Алави";
    else if (name == "Alderman") return "Олдерман";
    else if (name == "Allen") return "Аллен";
    else if (name == "Arya") return "Арья";
    else if (name == "Baker") return "Бейкер";
    else if (name == "Barb") return "Барб";
    else if (name == "Bartolucci") return "Бартолуччи";
    else if (name == "Bell") return "Белл";
    else if (name == "Blumel") return "Блюмель";
    else if (name == "Boon") return "Бун";
    else if (name == "Bowling") return "Боуллинг";
    else if (name == "Campbell") return "Кэмпбелл";
    else if (name == "Chen") return "Чен";
    else if (name == "Cherubini") return "Черубини";
    else if (name == "Collier") return "Колльер";
    else if (name == "Cotterell") return "Коттерелл";
    else if (name == "Davis") return "Дэвис";
    else if (name == "Eady") return "Эйди";
    else if (name == "Emslie") return "Эмсли";
    else if (name == "Field") return "Филд";
    else if (name == "Fukuda") return "Фукуда";
    else if (name == "Gaines") return "Гейнс";
    else if (name == "Ganus") return "Ганус";
    else if (name == "Gigliotti") return "Джильотти";
    else if (name == "Gillis") return "Гиллис";
    else if (name == "Glasco") return "Гласко";
    else if (name == "Glenn") return "Гленн";
    else if (name == "Gompert") return "Гомперт";
    else if (name == "Grenier") return "Гренье";
    else if (name == "Griffen") return "Гриффен";
    else if (name == "Haggerty") return "Хаггерти";
    else if (name == "Hammon") return "Хаммон";
    else if (name == "Harmer") return "Хармер";
    else if (name == "Harris") return "Харрис";
    else if (name == "Hatch") return "Хэтч";
    else if (name == "Hawkins") return "Хокинс";
    else if (name == "Heath") return "Хит";
    else if (name == "James") return "Джеймс";
    else if (name == "Kar") return "Кар";
    else if (name == "Keating") return "Китинг";
    else if (name == "Kriegler") return "Криглер";
    else if (name == "Kuhn") return "Кун";
    else if (name == "Lastimosa") return "Ластимоса";
    else if (name == "Lopez") return "Лопез";
    else if (name == "Lor") return "Лор";
    else if (name == "Louie") return "Луи";
    else if (name == "Lowis") return "Лоуис";
    else if (name == "Luo") return "Луо";
    else if (name == "Massey") return "Мэсси";
    else if (name == "McCandlish") return "Маккэндлиш";
    else if (name == "McCoy") return "Маккой";
    else if (name == "McLeod") return "Маклеод";
    else if (name == "Messerly") return "Мессерли";
    else if (name == "Miller") return "Миллер";
    else if (name == "Niebel") return "Нибель";
    else if (name == "Oh") return "О";
    else if (name == "Ojeda") return "Охеда";
    else if (name == "Onur") return "Онур";
    else if (name == "Peas") return "Пис";
    else if (name == "Pelayo") return "Пелайо";
    else if (name == "Pierce") return "Пирс";
    else if (name == "Porter") return "Портер";
    else if (name == "Rieke") return "Рике";
    else if (name == "Rosemeier") return "Роземайер";
    else if (name == "Roycewicz") return "Ройцевич";
    else if (name == "Rubin") return "Рубин";
    else if (name == "Rule") return "Рул";
    else if (name == "Sharrigan") return "Шарриган";
    else if (name == "Shiring") return "Ширинг";
    else if (name == "Sue") return "Сью";
    else if (name == "Turner") return "Тернер";
    else if (name == "Vinson") return "Винсон";
    else if (name == "Volker") return "Фолькер";
    else if (name == "Wang") return "Ванг";
    else if (name == "West") return "Вест";
    else if (name == "Yang") return "Янг";
    else if (name == "Zampella") return "Зампелла";
    else if (name == "Mejia") return "Мехия";
    else if (name == "Becerra") return "Бесерра";
    else if (name == "Castillo") return "Кастильо";
    else if (name == "Childress") return "Чайлдресс";
    else if (name == "Germann") return "Германн";
    else if (name == "Lara") return "Лара";
    else if (name == "Ovando") return "Овандо";
    else if (name == "Slanchik") return "Сланчик";
    else if (name == "Vo") return "Во";
    else if (name == "Garnett") return "Гарнетт";
    else if (name == "Smith") return "Смит";
    else if (name == "Troy") return "Трой";
    else if (name == "Carson") return "Карсон";
    // British names
    else if (name == "Abbot") return "Эббот";
    else if (name == "Adams") return "Адамс";
    else if (name == "Bartlett") return "Бартлетт";
    else if (name == "Boyd") return "Бойд";
    else if (name == "Boyle") return "Бойл";
    else if (name == "Bremner") return "Бремнер";
    else if (name == "Carlyle") return "Карлайл";
    else if (name == "Carver") return "Карвер";
    else if (name == "Cheek") return "Чик";
    else if (name == "Clarke") return "Кларк";
    else if (name == "Collins") return "Коллинс";
    else if (name == "Compton") return "Комптон";
    else if (name == "Connolly") return "Коннолли";
    else if (name == "Cook") return "Кук";
    else if (name == "Dowd") return "Доуд";
    else if (name == "Field") return "Филд";
    else if (name == "Fleming") return "Флеминг";
    else if (name == "Fletcher") return "Флетчер";
    else if (name == "Flynn") return "Флинн";
    else if (name == "Grant") return "Грант";
    else if (name == "Greaves") return "Гривз";
    else if (name == "Griffin") return "Гриффин";
    else if (name == "Harris") return "Харрис";
    else if (name == "Harrison") return "Харрисон";
    else if (name == "Heath") return "Хит";
    else if (name == "Henderson") return "Хендерсон";
    else if (name == "Hopkins") return "Хопкинс";
    else if (name == "Hoyt") return "Хойт";
    else if (name == "Kent") return "Кент";
    else if (name == "Lewis") return "Льюис";
    else if (name == "Lipton") return "Липтон";
    else if (name == "Macdonald") return "Макдональд";
    else if (name == "Maxwell") return "Максвелл";
	else if (name == "McQuarrie") return "Маккварри";
    else if (name == "Miller") return "Миллер";
    else if (name == "Mitchell") return "Митчелл";
    else if (name == "Moore") return "Мур";
    else if (name == "Murphy") return "Мёрфи";
    else if (name == "Murray") return "Мюррей";
    else if (name == "Pearce") return "Пирс";
    else if (name == "Plumber") return "Пламбер";
    else if (name == "Pritchard") return "Притчард";
    else if (name == "Rankin") return "Рэнкин";
    else if (name == "Reed") return "Рид";
    else if (name == "Ritchie") return "Ричи";
    else if (name == "Ross") return "Росс";
    else if (name == "Roth") return "Рот";
    else if (name == "Smith") return "Смит";
    else if (name == "Stevenson") return "Стивенсон";
    else if (name == "Stuart") return "Стюарт";
    else if (name == "Sullivan") return "Салливан";
    else if (name == "Thompson") return "Томпсон";
    else if (name == "Veale") return "Вил";
    else if (name == "Wallace") return "Уоллес";
    else if (name == "Wallcroft") return "Уолкрофт";
    else if (name == "Wells") return "Уэллс";
    else if (name == "Welsh") return "Уэлш";
    // Russian names
    else if (name == "Sasha Ivanov") return "Саша Иванов";
    else if (name == "Aleksei Vyshinskiy") return "Алексей Вышинский";
    else if (name == "Boris Ryzhkov") return "Борис Рыжков";
    else if (name == "Dima Tikhonov") return "Дима Тихонов";
    else if (name == "Oleg Kosygin") return "Олег Косыгин";
    else if (name == "Pyotr Bulganin") return "Пётр Булганин";
    else if (name == "Petya Malenkov") return "Петя Маленков";
    else if (name == "Alyosha Tarkovsky") return "Алёша Тарковский";
    else if (name == "Sergei Grombyo") return "Сергей Громбё";
    else if (name == "Viktor Kuznetsov") return "Виктор Кузнецов";
    else if (name == "Misha Podgorniy") return "Миша Подгорный";
    else if (name == "Borya Mikoyan") return "Боря Микоян";
    else if (name == "Anatoly Voroshilov") return "Анатолий Ворошилов";
    else if (name == "Kolya Shvernik") return "Коля Шверник";
    else if (name == "Nikolai Kalinin") return "Николай Калинин";
    else if (name == "Vladimir Brezhnev") return "Владимир Брежнев";
    else if (name == "Pavel Chernenko") return "Павел Черненко";
    else if (name == "Volodya Andropov") return "Володя Андропов";
    else if (name == "Yuri Nikitin") return "Юрий Никитин";
    else if (name == "Dmitri Petrenko") return "Дмитрий Петренко";
    else if (name == "Vanya Gerasimov") return "Ваня Герасимов";
    else if (name == "Mikhail Zhuravlev") return "Михаил Журавлёв";
    else if (name == "Ivan Lukin") return "Иван Лукин";
    else if (name == "Kostya Golubev") return "Костя Голубев";
    else if (name == "Konstantin Lebedev") return "Константин Лебедев";
    else if (name == "Aleksandr Vasilev") return "Александр Васильев";
    else if (name == "Yakov Glushenko") return "Яков Глушенко";
    else if (name == "Sasha Semenov") return "Саша Семёнов";
    else if (name == "Aleksei Ulyanov") return "Алексей Ульянов";
    else if (name == "Boris Yefremov") return "Борис Ефремов";
    else if (name == "Dima Chernyshenko") return "Дима Чернышенко";
    else if (name == "Oleg Stepanoshvili") return "Олег Степаношвили";
    else if (name == "Pyotr Demchenko") return "Пётр Демченко";
    else if (name == "Petya Avagimov") return "Петя Авагимов";
    else if (name == "Alyosha Murzaev") return "Алёша Мурзаев";
    else if (name == "Sergei Shkuratov") return "Сергей Шкуратов";
    else if (name == "Viktor Yakimenko") return "Виктор Якименко";
    else if (name == "Misha Masijashvili") return "Миша Масияшвили";
    else if (name == "Borya Shapovalov") return "Боря Шаповалов";
    else if (name == "Anatoly Ivashenko") return "Анатолий Ивашенко";
    else if (name == "Kolya Dovzhenko") return "Коля Довженко";
    else if (name == "Nikolai Turdyev") return "Николай Турдыев";
    else if (name == "Vladimir Sabgaida") return "Владимир Сабгайда";
	else if (name == "Pavel Svirin") return "Павел Свирин";
    else if (name == "Volodya Sarayev") return "Володя Сараев";
    else if (name == "Yuri Kiselev") return "Юрий Киселёв";
    else if (name == "Dmitri Bondarenko") return "Дмитрий Бондаренко";
    else if (name == "Vanya Chernogolov") return "Ваня Черноголов";
    else if (name == "Mikhail Voronov") return "Михаил Воронов";
    else if (name == "Ivan Afanasyev") return "Иван Афанасьев";
    else if (name == "Kostya Gridin") return "Костя Гридин";
    else if (name == "Konstantin Petrov") return "Константин Петров";
    else if (name == "Aleksandr Rykov") return "Александр Рыков";
    else if (name == "Yakov Shvedov") return "Яков Шведов";
    // Arab names
    else if (name == "Abdulaziz") return "Абдулазиз";
    else if (name == "Abdullah") return "Абдулла";
    else if (name == "Ali") return "Али";
    else if (name == "Amin") return "Амин";
    else if (name == "Bassam") return "Бассам";
    else if (name == "Fahd") return "Фахд";
    else if (name == "Faris") return "Фарис";
    else if (name == "Fouad") return "Фуад";
    else if (name == "Habib") return "Хабиб";
    else if (name == "Hakem") return "Хакем";
    else if (name == "Hassan") return "Хассан";
    else if (name == "Ibrahim") return "Ибрагим";
    else if (name == "Imad") return "Имад";
    else if (name == "Jabbar") return "Джаббар";
    else if (name == "Kareem") return "Карим";
    else if (name == "Khalid") return "Халид";
    else if (name == "Malik") return "Малик";
    else if (name == "Muhammad") return "Мухаммад";
    else if (name == "Nasir") return "Насир";
    else if (name == "Omar") return "Омар";
    else if (name == "Rafiq") return "Рафик";
    else if (name == "Rami") return "Рами";
    else if (name == "Said") return "Саид";
    else if (name == "Salim") return "Салим";
    else if (name == "Samir") return "Самир";
    else if (name == "Talib") return "Талиб";
    else if (name == "Tariq") return "Тарик";
    else if (name == "Youssef") return "Юссеф";
    else if (name == "Ziad") return "Зиад";
	else return name;
}

GetCaptainPriceName() { return "Капитан Прайс"; }
GetGazName() { return "Гас"; }
GetSSgtGriggsName() { return "Штаб-сержант Григгс"; }
GetLtVasquezName() { return "Лейтенант Васкес"; }
GetCaptainMacMillanName() { return "Капитан МакМиллан"; }
GetSgtKamarovName() { return "Сержант Камаров"; }
GetNikolaiName() { return "Николай"; }
GetViktorZakhaevName() { return "Виктор Захаев"; }
//--------------------------------------------------------------------
GetSgtWallcroftName() { return "Сержант Уолкрофт"; }
GetPvtGriffenName() { return "Рядовой Гриффен"; }
GetCptPelayoName() { return "Капитан Пелайо"; }
GetCptKeatingName() { return "Капитан Китинг"; }
GetLtVolkerName() { return "Лейтенант Волкер"; }
GetPvtMasseyName() { return "Рядовой Мэсси"; }
GetPvtRoycewiczName() { return "Рядовой Ройсевич"; }
GetPvtWestName() { return "Рядовой Уэст"; }
GetSgtScullyName() { return "Сержант Скалли"; }
GetCW4SmithName() { return "Уорент-офицер Смит"; }
GetSgtLovejoyName() { return "Сержант Лавджой"; }
GetPvtLootzName() { return "Рядовой Лутц"; }
GetMacName() { return "Мак"; }
GetSgtNewcastleName() { return "Сержант Ньюкасл"; }
GetSgtAremName() { return "Сержант Арэм"; }
GetSgtBartonName() { return "Сержант Бартон"; }

GetPrivateRank() { return "Рядовой "; }
GetCorporalRank() { return "Капрал "; }
GetSergeantRank() { return "Сержант "; }
GetLieutenantRank() { return "Лейтенант "; }
GetCaptainRank() { return "Капитан "; }