const earningIdeas = [
  'Offer micro-gig prompt engineering service on freelancing marketplaces.',
  'Create AI-assisted short-form content packages for local businesses.',
  'Build niche study notes and sell as digital products.'
];

export function generateChatResponse(message, interests = []) {
  const interestText = interests.length ? ` based on your interests in ${interests.join(', ')}` : '';
  return `LifeOS AI suggests${interestText}: Break your goal into 3 actions today. For "${message}", start with research, draft output, and publish.`;
}

export function generateVideoSummary(topic) {
  return {
    topic,
    keyPoints: [
      'Main thesis condensed into one actionable sentence.',
      'Three practical steps extracted from the content.',
      'One KPI to track progress this week.'
    ],
    notes: `Use this summary of ${topic} to revise in 10 minutes.`
  };
}

export function generateBook(topic) {
  return {
    title: `${topic}: Rapid Mastery Playbook`,
    chapters: [
      'Mindset and Fundamentals',
      'Execution Framework',
      'Portfolio and Monetization'
    ],
    quickSummary: `A concise guide to learn ${topic}, build projects, and monetize skills.`
  };
}

export function getRevenueIdeas() {
  return earningIdeas.map((idea, index) => ({
    id: index + 1,
    idea,
    executionPlan: ['Pick a niche', 'Create one sample output', 'Pitch to 10 clients']
  }));
}
