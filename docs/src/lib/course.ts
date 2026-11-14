import { siteUrl } from '@/lib/site';

// Structured data for the Learn Bluetooth Low Energy course. A page becomes a
// lesson by being listed here; the course workload adds up the listed lessons.

type Lesson = {
  url: string;
  position: number;
  minutes: number;
  teaches: string[];
};

type Module = {
  name: string;
  description: string;
  lessons: Lesson[];
};

const COURSE_URL = '/learn';
const COURSE_NAME = 'Learn Bluetooth Low Energy';
const COURSE_ID = `${siteUrl}${COURSE_URL}#course`;
const PROVIDER = { '@type': 'Organization', name: 'SimpleBLE', url: 'https://simpleble.org' };

const MODULES: Module[] = [
  {
    name: 'Module 1: First contact',
    description: 'Find the devices around you and read what they broadcast.',
    lessons: [
      {
        url: '/learn/what-is-bluetooth-low-energy',
        position: 1,
        minutes: 15,
        teaches: [
          'What Bluetooth Low Energy is',
          'How BLE differs from Bluetooth Classic',
          'How BLE devices exchange data through advertising and GATT',
          'How a program reaches the Bluetooth adapter',
        ],
      },
      {
        url: '/learn/central-and-peripheral',
        position: 2,
        minutes: 10,
        teaches: [
          'The central, peripheral, broadcaster and observer roles',
          'How a central finds and connects to a peripheral',
          'Telling peripherals from broadcasters in a scan',
        ],
      },
      {
        url: '/learn/scan-for-ble-devices',
        position: 3,
        minutes: 15,
        teaches: [
          'How a BLE scan discovers devices',
          'Scan found and scan updated events',
          'How long to scan',
          'Finding the closest device by signal strength',
        ],
      },
      {
        url: '/learn/read-ble-advertisements',
        position: 4,
        minutes: 15,
        teaches: [
          'The structure of BLE advertising data',
          'Reading names, service UUIDs, service data and manufacturer data',
          'Company identifiers and TX power',
        ],
      },
      {
        url: '/learn/rssi-signal-strength',
        position: 5,
        minutes: 15,
        teaches: [
          'What RSSI measures',
          'Why RSSI readings fluctuate',
          'Averaging RSSI readings',
          'Estimating distance from RSSI and TX power',
        ],
      },
    ],
  },
  {
    name: 'Module 2: Advertising',
    description: 'Make your computer visible and broadcast data of your own.',
    lessons: [],
  },
  {
    name: 'Module 3: Connections and GATT',
    description: 'Connect to a device and exchange data with it.',
    lessons: [],
  },
  {
    name: 'Module 4: Build a peripheral',
    description:
      'Serve GATT from your computer, build a heart rate service end to end, and connect your own central to it.',
    lessons: [],
  },
  {
    name: 'Module 5: Real devices',
    description:
      'Read a heart rate strap, test your nRF or ESP32 devkit, and keep a device connected with a reconnect loop.',
    lessons: [],
  },
  {
    name: 'Module 6: Debugging and shipping',
    description: 'Disconnects, error codes, HCI logs, addresses and privacy, OS permissions and concurrency.',
    lessons: [],
  },
];

function duration(minutes: number) {
  const hours = Math.floor(minutes / 60);
  const rest = minutes % 60;
  return `PT${hours ? `${hours}H` : ''}${rest || !hours ? `${rest}M` : ''}`;
}

function totalMinutes(lessons: Lesson[]) {
  return lessons.reduce((sum, lesson) => sum + lesson.minutes, 0);
}

function courseJsonLd(description: string) {
  const lessons = MODULES.flatMap((module) => module.lessons);

  return {
    '@context': 'https://schema.org',
    '@type': 'Course',
    '@id': COURSE_ID,
    name: COURSE_NAME,
    description,
    url: `${siteUrl}${COURSE_URL}`,
    provider: PROVIDER,
    isAccessibleForFree: true,
    inLanguage: 'en',
    educationalLevel: 'Beginner',
    offers: { '@type': 'Offer', category: 'Free', price: 0, priceCurrency: 'USD' },
    hasCourseInstance: {
      '@type': 'CourseInstance',
      courseMode: 'Online',
      courseWorkload: duration(totalMinutes(lessons)),
    },
    syllabusSections: MODULES.map((module) => ({
      '@type': 'Syllabus',
      name: module.name,
      description: module.description,
      ...(module.lessons.length > 0 ? { timeRequired: duration(totalMinutes(module.lessons)) } : {}),
    })),
  };
}

function lessonJsonLd(lesson: Lesson, title: string, description: string) {
  const url = `${siteUrl}${lesson.url}`;

  return [
    {
      '@context': 'https://schema.org',
      '@type': 'LearningResource',
      '@id': `${url}#lesson`,
      name: title,
      description,
      url,
      learningResourceType: 'Lesson',
      educationalLevel: 'Beginner',
      inLanguage: 'en',
      isAccessibleForFree: true,
      provider: PROVIDER,
      isPartOf: { '@id': COURSE_ID },
      position: lesson.position,
      teaches: lesson.teaches,
      timeRequired: duration(lesson.minutes),
    },
    {
      '@context': 'https://schema.org',
      '@type': 'BreadcrumbList',
      itemListElement: [
        { '@type': 'ListItem', position: 1, name: 'Docs', item: siteUrl },
        { '@type': 'ListItem', position: 2, name: COURSE_NAME, item: `${siteUrl}${COURSE_URL}` },
        { '@type': 'ListItem', position: 3, name: title, item: url },
      ],
    },
  ];
}

// Returns the JSON-LD documents for a course page, or none for any other page.
export function getCourseJsonLd(page: { url: string; data: { title: string; description?: string } }) {
  const description = page.data.description ?? '';
  if (page.url === COURSE_URL) return [courseJsonLd(description)];

  const lesson = MODULES.flatMap((module) => module.lessons).find((item) => item.url === page.url);
  return lesson ? lessonJsonLd(lesson, page.data.title, description) : [];
}
