<h1>Менеджер скриншотов</h1>

Данная программа создана для удобства управления большими коллекциями изображений и ведения публичных страниц в соцсетях, публикующих материалы из таких коллекций. Основная область применения программы – коллекции различных кадров с субтитрами и использование их для публикации в ВК, поэтому в описании программы рассматривается именно этот случай. При желании возможно использовать данную программу и для других разнообразных изображений, имеющих какое-либо текстовое описание. 

<h2>Основные используемые понятия</h2>

<h3>Запись</h3>

Запись в контексте данной программы обозначает JSON-объект, содержащий информацию о наборе изображений и соответствующий данному набору текст. Для каждого изображения в записи отмечаются имя файла на диске, идентификатор в альбоме ВК и URL-адрес файла в ВК. Каждая запись при этом может быть отмечена как публичная или скрытая – этот параметр используется для отбора записей для публикации постов.

<h3>Журнал</h3>

Журнал представляет из себя JSON-файл, содержащий список записей. Дополнительно журнал содержит информацию о названии фильма или аниме, служившего источником для кадров, а также идентификатор соответствующего альбома в ВК, текст для подписывания кадров в альбоме и более общее название, посредством которого можно объединять несколько журналов, к примеру, представляющих отдельные сезоны одного сериала.

<h3>Хэштег</h3>

Хэштег представляет собой слово, предваряемое знаком # или & и используемое для категоризации записей по темам, изображённым объектам или другим произвольным признакам (примеры: #пример, #тема_2, &кот). Хэштеги включаются пользователем в текстовую часть записи после основного текста, при этом количество хэштегов для каждой записи не ограничено. Хэштеги делятся на два основных типа: символ # обозначает простой тег, а символ & используется для «слабых» тегов, которые могут относиться к теме косвенно.

<h2>Функциональные режимы</h2>

<h3>Создание нового журнала</h3>

<h4>Назначение и функционал</h4>

Данный режим предназначен для объединения информации об изображениях в записи и сохранения их в журнал. В ходе работы в данном режиме пользователь проходит по заготовленному списку цитат и прикрепляет к каждой из них один или несколько соответствующих кадров. После прохождения всех кадров из выбранной папки созданные записи сохраняются в новый журнал.

<h4>Интерфейс</h4>

![image](https://github.com/user-attachments/assets/c183192e-8b4b-498d-96ab-27e5b394e5e2)

Рисунок 1 – Интерфейс режима создания журнала

Как можно наблюдать на рисунке 1, большую часть экрана занимает изображение одного из кадров, расположенных в папке screenshots\%FOLDER_NAME. Ниже располагается слайдер, отражающий прогресс обработки кадров пользователем. Под слайдером расположено текстовое поле, в котором отображаются цитаты, прочитанные в файле docs\%FOLDER_NAME.txt.

В нижней части экрана отображается информационная строка, в которой указывается общее количество цитат и кадров, а также номер текущей цитаты и текущего кадра. Если к будущей записи добавлено больше одного кадра, то отображается диапазон их номеров.

Кнопка «Пропустить»: не включает в запись текущий кадр и показывает следующий.

Кнопка «Добавить»: добавляет дополнительный кадр к будущей записи и показывает его.

Кнопка «Назад»: показывает предыдущий кадр. При нажатии Ctrl превращается в кнопку «Отмена» и позволяет отменить последнее действие в случае ошибки.

Кнопка «Готово»: создаёт запись, содержащую выбранный набор кадров и текущую цитату, и показывает следующий кадр и следующую цитату. При достижении конца списка кадров нажатие на эту кнопку производит автоматическое сохранение нового журнала и обновление списка цитат на случай, если в него вносились изменения.

На верхней панели инструментов располагается зелёная кнопка с галочкой, с помощью которой можно сразу помечать записи как скрытые, после чего кнопка станет красной до перехода к созданию к следующей записи. По умолчанию все записи считаются публичными.

<h4>Требования</h4>

1. В конфигурационном файле должны быть указаны корректные пути configs, docs и screenshots, а также токен авторизации access_token и идентификатор группы group_id.

2. Выбранная папка screenshots\%FOLDER_NAME должна содержать хотя бы одно изображение.

3. Количество изображений и их порядок в папке %FOLDER_NAME должны совпадать с количеством и порядком фотографий в альбоме с названием %FOLDER_NAME, расположенном в группе group_id.

4. В папке docs должен лежать текстовый документ с названием формата %FOLDER_NAME.txt.

5. Количество строк в текстовом документе %FOLDER_NAME.txt должно быть не меньше 1 и не больше, чем количество кадров в папке %FOLDER_NAME.

<h3>Чтение и редактирование журналов</h3>

<h4>Назначение и функционал</h4>

Данный режим предназначен для просмотра и редактирования журналов, созданных в рассмотренном выше режиме. Основная цель редактирования – уточнение текстовой информации, изменение параметра публичности отдельных записей и внесение в текст хэштегов для их категоризации. Можно одновременно открыть и редактировать как один, так и несколько журналов, при этом все внесённые изменения сохраняются в соответствующие файлы. Для редактирования удобно рассматривать записи по одной, а для чтения журналов в целом можно отобразить их списком или галереей.

<h4>Интерфейс</h4>

Основной интерфейс в данном режиме похож на интерфейс создания журнала, но имеет несколько интуитивно понятных отличий: вместо кнопок «Добавить» и «Готово» используются кнопки «Листать» и «Далее». Просмотр записей в журнале осуществляется кнопками «Назад» и «Далее», а если запись содержит несколько изображений, их можно перелистывать кнопкой «Листать». Слайдер в данном режиме активен, с его помощью можно осуществлять быстрый переход между записями. В информационной строке в нижней части экрана указывается номер текущей записи, общее количество записей, а также количество кадров в записи и номер текущего кадра. 

При просмотре записи можно редактировать текст, а также менять её статус (скрытый или публичный). Внесённые изменения сохранятся в записи, если нажать кнопку «Далее». При нажатии других кнопок («Назад» или «Листать») или при использовании слайдера внесённые изменения сбросятся.

Главное нововведение по отношению к интерфейсу режима создания журнала – это возможность использования панели хэштегов, доступной при зажатии клавиши Ctrl. Панель хэштегов замещает верхние элементы интерфейса, но оставляет возможность редактирования текста. На панели хэштегов в виде таблицы располагаются кнопки со всеми доступными тегами. При наличии хэштега в журнале она изображается плоской, а после названия тега указывается количество его использований в журнале. Чтобы добавить к текущей записи хэштег со знаком #, нужно нажать на кнопку левой клавишей мыши, а чтобы добавить хэштег со знаком & -- правой клавишей мыши. Нажатие на кнопку колёсиком мышки позволяет провести быстрый поиск по хэштегу, причём фильтр результатов по хэштегам свободно сочетается с другими методами поиска. 
 
![image](https://github.com/user-attachments/assets/fe00060a-69cb-4e17-8640-d2d16385af95)

Рисунок 2 – Интерфейс режима чтения журналов: панель хэштегов

На панели инструментов доступны кнопки для отображения записей в виде списка или галереи. В обоих случаях на экране доступно поле для поиска записей по тексту, рядом с ним расположены две кнопки: для активации и сброса поиска. Справа от них доступны настройки: количество записей, отображаемых на странице, и номер текущей страницы.
 
![image](https://github.com/user-attachments/assets/68019416-a7e3-4a6d-aa6a-123b5bebb8f9)

Рисунок 3 – Отображение записей в виде списка

В виде списка каждая запись представлена порядковым номером, первым кадром и текстом с хэштегами. В виде списка удобно проверять текст и обозревать результаты поиска по хэштегам или текстовым запросам. Дважды кликнув на запись, можно открыть её в основном виде для внесения изменений.
 
![image](https://github.com/user-attachments/assets/bb70d040-c83a-4329-a35b-db7364c8e5a4)

Рисунок 4 – Отображение записей в виде галереи

В виде галереи каждая запись представлена первым кадром, под которым располагаются галочка и короткий текст с указанием количества кадров. Наличие или отсутствие галочки соответственно обозначает публичность или скрытость записи. В виде галереи удобно искать нужную запись визуально и отслеживать, какие из них скрыты.

Помимо прочего, в меню «Действия» доступны опции «Только публичные» и «Только скрытые», которые можно использовать для выбора записей по параметру их публичности.

<h4>Требования</h4>

1. Открываемые журналы должны содержать корректные данные.

2. Для использования хэштегов в конфигурационном файле по ключу «hashtags» должно быть указано расположение файла с данными о хэштегах.

3. Для отображения кадров из журнала %JOURNAL_NAME.json в локальном режиме соответствующие файлы должны располагаться в папке screenshots\% JOURNAL_NAME.

4. Для загрузки кадров из журнала %JOURNAL_NAME.json необходимо убедиться, что опция «Работа оффлайн» в меню «Режим работы» отключена и установлено интернет-соединение.

<h3>Создание списка цитат из субтитров</h3>

<h4>Назначение и функционал</h4>

Данный режим позволяет автоматизировать получение текстовых цитат из кадров с субтитрами. Он предназначен не для непосредственной работы с журналами, а для упрощения подготовки текстовых данных, которые необходимы для запуска режима создания журнала. 

В ходе работы в данном режиме пользователь проходит по изображениям из выбранной папки, а программа предлагает подходящую строку субтитров. В ряде случаев (к примеру, из-за ошибок во временных метках) автоматически найти необходимые строки не удаётся, тогда пользователь может легко найти более подходящую, используя либо временную очерёдность, либо алфавитный порядок строк.

После прохождения всех кадров из выбранной папки выбранные строки сохраняются в текстовый документ в папке docs.

<h4>Интерфейс</h4>

Основной интерфейс схож с интерфейсами создания или чтения журналов. Кнопками «Готово» и «Назад» осуществляется последовательный переход между изображениями из открытой папки. Для выбора строки в ручном режиме предназначена кнопка «Подгрузка субтитров» на панели инструментов. 

В режиме подгрузки субтитров программа открывает файл субтитров, соответствующий текущему кадру, и активируются слайдер и кнопки «Предыдущий» и «Следующий», посредством которых можно перебирать обнаруженные в файле строки в порядке их очерёдности – это особенно удобно в том случае, когда автоматически определённая строка находится рядом с необходимой, что часто легко понять по контексту. Если таким образом найти необходимую строку не удаётся, можно отобразить содержимое файла субтитров в виде списка реплик, отсортированных в алфавитном порядке, и выбрать правильную двойным кликом.

![image](https://github.com/user-attachments/assets/f95092b4-3d71-483d-ab39-efeda3b9c404)

Рисунок 5 – Отображение строк из файла субтитров в виде списка

<h4>Требования</h4>

1. Скриншоты в открытой папке %FOLDER_NAME должны иметь названия формата %IMAGE_NAME-h-mm-ss-zzz, где «h-mm-ss-zzz» обозначает временную метку с точностью до миллисекунд.

2. Папка subs также должна содержать подпапку с названием %FOLDER_NAME.

3. В папке subs\%FOLDER_NAME должны находиться файлы субтитров формата .ass с названиями, соответствующими %IMAGE_NAME (без временных меток).

<h3>Публикация постов</h3>

<h4>Назначение и функционал</h4>

Данный режим позволяет выбирать заданное количество записей и публиковать их в публичной странице в ВК в виде отложенных постов. Посты отправляются на выбранную дату, при этом первый отправляется на выбранное время, а последующие идут с заданным интервалом. Посты можно выбирать как случайно, так и вручную, при этом можно задать поиск по хэштегам, по тексту, по сериалу, по дате последней публикации или по количеству кадров в записи.

Кроме того, данный режим позволяет на основе хэштегов создавать опросы и также публиковать их в публичной странице. Также существует возможность создания постов на основе набора хэштегов из опроса.

<h4>Интерфейс</h4>

![image](https://github.com/user-attachments/assets/d1685ebe-f937-4c76-8528-5187be31629c)

Рисунок 6 – Интерфейс режима публикации постов

На основном экране режима выбора постов в верхней части представлены основные настройки: дата и время публикации, количество постов и интервал между ними. В следующем ряду находятся настройки фильтрации записей для отсева недавно публиковавшихся записей или сериалов, а также для выбора записей с одним или несколькими кадрами. Ниже находится кнопка «Генерировать», нажатие на которую готовит набор случайных записей для публикации с заданными параметрами, а сами сгенерированные записи отображаются ниже.

Выбранные записи представлены своим порядковым номером, своими кадрами и текстом, а также для каждой из них отображается прошедшее время с их последней публикации, если запись публиковалась. Слева располагаются кнопки:

1. Время публикации: показывает время публикации в подсказке и при необходимости позволяет установить его вручную.

2. Случайный выбор: меняет запись на случайную с учётом выбранных фильтров.

3. Выбор по номеру: позволяет установить запись по её порядковому номеру.

4. Поиск по списку: отображает записи в виде списка, позволяет найти и установить нужную вручную.

5. Сдвинуть вниз: меняет запись местами со следующей и используется для изменения порядка постов.

<h4>Требования</h4>

1. В конфигурационном файле должен быть токен авторизации access_token, идентификатор группы group_id, идентификатор паблика public_id, а также путь public_records. Для полноценной работы также необходимо указать корректные пути logs, poll_logs и hashtags.

2. Для запуска режима необходим файл public_records, который создаётся из существующих журналов путём компиляции всех публичных записей в один список. Этот файл можно сгенерировать, нажав «Компилировать» в меню «Действия».

