from __future__ import print_function
from collections import defaultdict, deque
from operator import itemgetter
from nltk.corpus import wordnet as wn

def compute_distance(edge_type):
	if edge_type == 0:
		return 1.0
	elif edge_type == 1:
		#return 0.5
		return 1.0
	elif edge_type == 2:
		#return 0.5
		return 1.0
	else:
		#return 0.2
		return 1.0

def find_siblings(syn):
	siblings = set()
	parents = syn.hypernyms()
	if parents:
		for parent in parents:
			siblings.update(parent.hyponyms())
		siblings.remove(syn)
	return siblings

def find_inst_siblings(syn):
	siblings = set()
	parents = syn.instance_hypernyms()
	if parents:
		for parent in parents:
			siblings.update(parent.instance_hyponyms())
		siblings.remove(syn)
	return siblings

def find_combined_siblings(syn):
	siblings = set()
	parents = syn.hypernyms()
	if parents:
		for parent in parents:
			siblings.update(parent.hyponyms())
			siblings.update(parent.instance_hyponyms())
		siblings.remove(syn)
	parents = syn.instance_hypernyms()
	if parents:
		for parent in parents:
			siblings.update(parent.hyponyms())
			siblings.update(parent.instance_hyponyms())
		siblings.remove(syn)
	return siblings

def bfs(root, visited):
	q = deque()
	new_visited = []
	q.append(root)
	visited.add(root)
	new_visited.append(root)

	while q:
		node = q.popleft()
		for edge in node.edges:
			if edge not in visited:
				q.append(edge)
				visited.add(edge)
				new_visited.append(edge)

	return new_visited

class WordOccurrence(object):
	def __init__(self, word, position):
		self.word = word
		self.senses = wn.synsets(word, 'n')
		self.edges = { sense : set() for sense in self.senses }
		self.position = position

	def __eq__(self, other):
		if isinstance(other, WordOccurrence):
			return self.word == other.word and self.position == other.position
		else:
			return NotImplemented

	def __hash__(self):
		return hash((self.word, self.position))

	def __str__(self):
		return '(%s,%s)' % (self.word, self.position)

	def __repr__(self):
		return self.__str__()

	@staticmethod
	def add_edge(lhs_occ, lhs_sense, rhs_occ, rhs_sense, dist):
		lhs_occ.edges[lhs_sense].add((rhs_occ, rhs_sense, dist))
		rhs_occ.edges[rhs_sense].add((lhs_occ, lhs_sense, dist))

	@staticmethod
	def remove_edge(lhs_occ, lhs_sense, edge):
		lhs_occ.edges[lhs_sense].remove(edge)
		edge[0].edges[edge[1]].remove((lhs_occ, lhs_sense, edge[2]))

class WordVertex(object):
	def __init__(self, word):
		self.word = word
		self.edges = set()

	def __eq__(self, other):
		if isinstance(other, WordVertex):
			return self.word == other.word
		else:
			return NotImplemented

	def __hash__(self):
		return hash(self.word)

	def __str__(self):
		return '(%s)' % (self.word,)

	def __repr__(self):
		return self.__str__()

	@staticmethod
	def add_edge(lhs, rhs):
		lhs.edges.add(rhs)
		rhs.edges.add(lhs)

f_hyper = lambda s: s.hypernyms()
f_hypo = lambda s: s.hyponyms()

f_inst_hyper = lambda s: s.instance_hypernyms()
f_inst_hypo = lambda s: s.instance_hyponyms()

class LexicalChainer(object):

	def __init__(self):
		self.occurrences = { }
		self.rev_index = defaultdict(set)
		self.word_senses = { }

	def build_disambiguation_graph(self, words):
		metachains = defaultdict(set)
		for position, word in enumerate(words):
			word = word.lower()
			occ = WordOccurrence(word, position)
			if occ.senses:
				self.occurrences[position] = occ
				self.rev_index[word].add(position)
				for sense in occ.senses:
					related = { sense : 0 }
					related.update({ x : 1 for x in sense.closure(f_hyper) })
					related.update({ x : 1 for x in sense.closure(f_inst_hyper) })
					#related.update({ x : 2 for x in sense.closure(f_hypo) })
					related.update({ x : 2 for x in sense.closure(f_hypo, 5) })
					related.update({ x : 2 for x in sense.closure(f_inst_hypo) })
					related.update({ x : 3 for x in find_siblings(sense) })
					related.update({ x : 3 for x in find_inst_siblings(sense) })
					#related.update({ x : 3 for x in find_combined_siblings(sense) })
					for metasense, edge_type in related.iteritems():
						if metasense in metachains:
							dist = compute_distance(edge_type)
							for metaocc in metachains[metasense]:
								WordOccurrence.add_edge(occ, sense, metaocc, metasense, dist)
					metachains[sense].add(occ)
			#if position % 100 == 0:
			#	print('progress:', float(100 * position) / len(words), words[position + 1] if position + 1 < len(words) else None)

	def wsd(self):
		for word, positions in self.rev_index.iteritems():
			for fpos in positions:
				break
			senses = self.occurrences[fpos].senses
			scores = defaultdict(int)
			for sense in senses:
				for position in positions:
					edge_list = self.occurrences[position].edges[sense]
					scores[sense] += sum(map(itemgetter(2), edge_list))
			best_sense = senses[0]
			best_score = scores[best_sense]
			for sense in senses[1:]:
				score = scores[sense]
				if score > best_score:
					best_score = score
					best_sense = sense
			#print(word, positions, scores, max(scores.iteritems(), key=itemgetter(1)))
			self.word_senses[word] = best_sense

	def prune_disambiguation_graph(self):
		for word, positions in self.rev_index.iteritems():
			for fpos in positions:
				break
			senses = self.occurrences[fpos].senses
			best_sense = self.word_senses[word]
			for sense in senses:
				if sense != best_sense:
					for position in positions:
						occ = self.occurrences[position]
						edges = occ.edges
						edge_list = occ.edges[sense]
						while edge_list:
							for edge in edge_list:
								break
							WordOccurrence.remove_edge(occ, sense, edge)

	def trim_disambiguation_graph(self):
		for word, positions in self.rev_index.iteritems():
			sense = self.word_senses[word]
			for position in positions:
				occ = self.occurrences[position]
				edge_list = occ.edges[sense]
				if len(edge_list) > 100:
					while edge_list:
						for edge in edge_list:
							break
						WordOccurrence.remove_edge(occ, sense, edge)

	def condense_graph(self):
		condensed_graph = { word : WordVertex(word) for word in self.rev_index }
		for word, positions in self.rev_index.iteritems():
			vertex = condensed_graph[word]
			sense = self.word_senses[word]
			for position in positions:
				occ = self.occurrences[position]
				edge_list = occ.edges[sense]
				for edge in edge_list:
					next_word = edge[0].word
					if next_word != word:
						WordVertex.add_edge(vertex, condensed_graph[next_word])
		return condensed_graph

	def build_chains(self):
		graph = self.condense_graph()

		offsets = []
		for word in graph:
			sense = self.word_senses[word]
			offsets.append((sense.offset, word))
		offsets.sort()

		visited = set()
		for _, word in offsets:
			vertex = graph[word]
			if vertex not in visited:
				chain = bfs(vertex, visited)
				if len(chain) > 1:
					print(chain)

		print('compare', len(offsets), len(visited))

text = 'The car is black just like the President of Romania Daft Punk have released their new hit single Gangnam Style Korean Starcraft 2 players are really cocky'
text = '''Sympathetic mirage mirage electricity aggregations aggregations Casey impoverishment Elec Elec Loeb Loeb piling piling ornate bacon FIGURES FIGURES Townsend whaling whaling ruthless ruthless Unemployed Unemployed Unemployed pericardial pericardial climbed climbed etiology etiology Hydrogen Hydrogen topography topography MEXICO anions anions symphony symphony symphony music standardized standardized Deutschland Deutschland Emerson Emerson blade centralization centralization organized organized preferably preferably hog Bates eugenics eugenics eugenics types types Guild Serbia Serbia Serbia Serbia deutsche deutsche signatories signatories signatories democratic democratic democratic Jules Jules murals murals murals Ludwig Ludwig Hickory Hickory schools schools diatoms diatoms diatoms ingot ingot underfoot chlorine chlorine shelves musicians loudness loudness Amended Amended Amended Natasha Natasha Natasha trek trek trek commissure Gradually Gradually Chicago Chicago dozed dozed Theo condensers condensers condensers Russo Russo Russo Texans Texans temperamental temperamental temperamental Cobbett tenaciously textbooks textbooks textbooks lurched lurched Abyssinian Abyssinian Composers Composers ambitions ambitions score efferent efferent lapping lapping welche welche canyon canyon Unquestionably sulcus sulcus contestants contestants Tomato Tomato Hapsburg Hapsburg conditioned conditioned player Brookings Brookings Brookings campaigning rapprochement rapprochement rapprochement principals ownership ownership Spalding Spalding Thank remedial remedial Connecticut Conservation Conservation fiduciary fiduciary Asiatic monogamy monogamy manufacturing manufacturing benches suggestions homesteads homesteads slightly crescendo crescendo crescendo stockholder stockholder abstractions Ende Homestead Homestead Homestead alloys alloys alloys safeguard equalization equalization County textbook textbook Charley Charley reorganization reorganization reorganization beamed disorganization disorganization skate Santayana Santayana Santayana Santayana Debussy Debussy Debussy Debussy consolidation consolidation Tap Organized Organized Tag attic attic Governmental Governmental Governmental shirts Crowell Crowell dwindling dwindling dusty instar Davis Hello Hello shouldered scratched Viennese Viennese Viennese Retail Retail valorem valorem Federal Federal soup Symphony Symphony Symphony rayon rayon rayon rayon buttered buttered jig jig BRITAIN thump decreased decreased parades parades Socialist Socialist Socialist Deutsche Deutsche Socialism Socialism Socialism shouted shouted aluminum aluminum aluminum hickory hickory legume legume legume coagulation quartet quartet quartet Sheppard Bermuda Added MARCH Barbour demoralization demoralization Foch Foch Foch Foch bumps bumps scarred scarred Triple routes routes Sanford RULE Mozart Mozart perpetuation slashed slashed fooled fooled favors Suggest Suggest afternoons afternoons horsepower horsepower horsepower horsepower Balinese Balinese Balinese handicraft handicraft nots nots Important Comstock Comstock opportunist opportunist opportunist synchronous synchronous synchronous MOVEMENT MOVEMENT ACTIVITIES ACTIVITIES ACTIVITIES Assets Assets Lexington BOARD BOARD Curriculum Curriculum Curriculum bureau bureau bureau jumper jumper Securities Securities Cosmos Cosmos Czechoslovak Czechoslovak Czechoslovak Delaware reflex straighten straighten newspaper newspaper Refer Refer Refer Nashville Nashville unsaturated unsaturated unsaturated barber bunks bunks bunks steamed steamed Diesel Diesel Diesel Diesel Jahrhunderts Jahrhunderts TEXAS TEXAS onethird Aunt Aunt Baltic Parole decomposes clam Expressed Expressed composers composers transformers transformers transformers poise poise Cash Cash Ripley Ripley standpoint standpoint standpoint Serge Serge legged Finley Finley rumbled rumbled fascists fascists fascists fascists gong gong hobbies hobbies hobbies watched cream cream standardize standardize standardize legislatures legislatures asteroids asteroids upkeep upkeep upkeep Farley Farley chewing Arturo Arturo Arturo blockade dodged dodged USED Crockett Crockett revolutionized revolutionized Cong Cong brimmed enactment Aus Aus indoors indoors coyote coyote Jahrhundert Jahrhundert industrialists industrialists industrialists industrialists huge freshly waved plant Emil Emil Gilchrist Gilchrist Briand Briand Briand Briand broadcasting broadcasting fundamental Llewellyn courses upstairs upstairs Erosion Erosion Erosion Lowest Riggs Riggs armaments armaments abnormally abnormally bumped bumped bumped lingual lingual Chesterton Chesterton Chesterton flimsy exponents exponents mural Alcohol vitally vitally odors odors scrawled scrawled Club Club ADMINISTRATION equaled equaled blackboard blackboard blackboard unused raucous raucous plan ext Cost Carolinas golf golf Explain Explain Palestine reels technic technic technic technic Confederate Dirk Dirk downpour downpour rumored rumored Amalgamated Amalgamated Amalgamated jade safeguarded safeguarded safeguarded baritone baritone baritone sparsely sparsely Mendelssohn Mendelssohn opal Wilde Jour Jour Jour loan smashing smashing Totals Totals Trail McBride McBride patrolled patrolled squirrel adaptable adaptable adaptable hostess enrollment enrollment enrollment Misc Misc von von premonition premonition rabid canned canned canned feces feces feces cabins stunt stunt Landor Landor cuffs cordon Dakota Dakota Dakota cronies cronies measurements measurements Hindenburg Hindenburg Hindenburg Hindenburg Franz Franz Pact Pact Pact idealistic idealistic idealistic Riviera Riviera biennial transients transients Pollock enthusiasms enthusiasms Used Used gases gases spat spat Natchez Natchez realists realists realists Buena paired Astor Astor pineal pineal yelled yelled overland overland stiffly Dividends Dividends Ownership Ownership meals expressionless expressionless pedestrians pedestrians Ritter Ritter menace menace Jim Jim highways highways churned churned recitals pupillary pupillary northwestern northwestern Seattle Seattle Bankruptcy licenses Spokane Spokane Spokane Reed dentin dentin dentin SECONDARY SECONDARY Draw Eskimo Eskimo Rifles billboards billboards slim slim Rosie Phelps Phelps mumbled mumbled insistent insistent insistent pigs pigs tomato tomato tomato nickel nickel nickel photoelectric photoelectric photoelectric photoelectric strychnine strychnine volts volts volts excretory hobby hobby derelict derelict Wholesale Wholesale Wholesale snails maturities maturities maturities realize realize reconstruction reconstruction San installment installment ruefully ruefully Arkansas Arkansas Garnett Garnett Platinum Platinum SCALE SCALE SCALE intangible intangible Treasury overproduction overproduction overproduction hilarious hilarious Teil Teil Teil loomed loomed Octavian Octavian Brenner Jump Jump Calvert staggered vetoed vetoed condensate condensate condensate accompaniment accompaniment AAA AAA encyclopedias encyclopedias encyclopedias mills Somebody mechanical rouge taps taps Kampf Kampf Kampf Kampf championed championed buttons Campion autos autos Salaries defenseless taxes Gilmore Gilmore Guidance Guidance coils coils coils disgruntled disgruntled disgruntled wiry seaboard seaboard drum Holyoke Holyoke Holyoke Centennial Centennial Bayou torsion torsion Ges Ges Yunnan Yunnan Yunnan Gee rioting inductance inductance inductance inductance Slightly Slightly grinning agonized Cummins apricots lengthening Loans Diplomatic Diplomatic tragically tragically overalls overalls overalls Hawaiians Hawaiians magnetization magnetization bananas bananas Corcoran Corcoran Clemente RESOURCES RESOURCES chairs garage garage garage Vladivostok Vladivostok Vladivostok liquidation liquidation Alloys Alloys Alloys Betsy Betsy salaries salaries Montana Montana salaried salaried invertebrates invertebrates Utopia loans loans drumming Sch Sch stocky stocky stocky Longmans Longmans Longmans whistles whistles cadet cadet protozoa protozoa protozoa expectantly expectantly fiscal fiscal lowered lowered disbursement Prelude Prelude Hygiene Hygiene Hygiene AGE AGE McClure McClure fireplaces fireplaces bewildered nigger nigger resorption resorption Bigger Bigger Shea Blanchard Scholastic auditing auditing experimentation experimentation experimentation plow alligator sweated lorry lorry lorry Walsh Mussolini Mussolini Mussolini Mussolini Abe Abe Gazette antislavery antislavery Marconi Marconi Marconi operatic operatic complacent Curves Curves greed claimant Colonial sludge sludge sludge recreational recreational recreational SPRING SPRING coffee Rubber Rubber Rubber Frankfurter Frankfurter Frankfurter logs logs Aside Aside Antoine passengers diesen diesen diesen Cereal Cereal Cereal Salzburg incisors incisors JAPAN JAPAN blossomed Tests Tests Tests stiffened stiffened Uncle Barrie Barrie Barrie upshot luncheon luncheon luncheon cereal cereal guild Association Association colonizing Magyars Magyars Magyars Correct Econ Econ correlating correlating correlating unsecured unsecured alumni alumni alumni scavenging scavenging scavenging swamp dredged dredged brief reactance reactance reactance reactance Armistice Armistice Armistice Kiel Kiel Janeiro Janeiro Bldg Bldg Bldg Haitian Haitian slack Armstrong gears gears gears Jackson arthropods arthropods cooperation cooperation peacefully Clerical sardines Board Board accentuate accentuate Beers Beers Dried Dried Cleveland Cleveland cylinder cylinder dumped dumped dizzy tugged Pal Harrisburg Harrisburg Pratt raid raid raid fuss fuss gunboat gunboat Tribune Tribune etiological etiological Duff athletic athletic transformer transformer transformer starboard openings McKee McKee starvation starvation Caldwell Tiny Tiny idealists idealists idealists Len Len violinist violinist violinist School School constructive constructive outlays outlays Swing Swing banked banked newspapers newspapers balances balances conservation conservation sagging sagging sagging Volk Volk Volk Cream Cream Catalina Catalina oboe oboe oboe Largely Largely Nadia Nadia aristocratic idealist idealist quilts emergencies automobiles automobiles automobiles Biennial Biennial Mencius Mencius yolks prairie prairie Slowly Slowly Cushman Cushman cynically cynically inauguration inauguration Deal Deal SOCIAL SOCIAL Rivera Rivera penalized penalized penalized Tientsin Tientsin Tientsin obsolescence obsolescence obsolescence club club publicist publicist stimulating clothing astigmatism astigmatism gonads gonads gonads southern drastic drastic drastic crepe crepe crepe nosed foreclosure foreclosure Pleistocene Pleistocene Pleistocene Abyssinia Bolivia Bolivia hydrogenation hydrogenation hydrogenation hydrogenation smash smash smash lumbering lumbering placate placate chuckled chuckled concerts concerts nematode nematode nematode Carolina Manchurian Manchurian Manchurian Manchurian happenings happenings happenings selections selections Travelers Travelers Cooperative Cooperative Cooperative serfs dentistry dentistry dentistry tennis tennis Courthouse Courthouse jugs waned waned Entente Entente Entente Entente Standardization Standardization Standardization occlusal occlusal occlusal Winnipeg Winnipeg CALIFORNIA Danville Danville Variability Variability terrific heaters heaters heaters algebraic algebraic Occurrence Occurrence Celeste Celeste conservatively conservatively conservatively Yukon Yukon Cossack Cossack interstate interstate interstate SCHOOL SCHOOL reorganized reorganized ramus ramus pituitary pituitary pituitary pouches Jock recalcitrant recalcitrant Bolsheviks Bolsheviks Bolsheviks skyscraper skyscraper skyscraper Gluck Gluck sess sess Alcott Alcott Alcott jerks jerks Floyd dictator Schools masonry Realizing Realizing centuries squatted squatted favorites Gases Gases greetings slanting tirade ruler Occidental Occidental Grande unearthed unearthed Chilean Chilean balked balked credits credits wurden wurden wurden Overland Overland Overland Brigham Brigham NUMBERS NUMBERS Highways Adaptation Adaptation sniffed sniffed Fascists Fascists Fascists Fascists mathematics expropriation expropriation esthetic esthetic esthetic profits quadrature fervor twothirds bombarded Gwen Gwen Hobson Hobson Inasmuch Inasmuch consumers consumers consumers apologized Tammany Tammany Tammany Seldom Borden Borden ultra outgrowth outgrowth musically musically STATION STATION Undoubtedly swoop swoop lowering factories factories factories stampede stampede Coolidge Coolidge Coolidge Coolidge Orchestra Orchestra Orchestra Interstate Interstate Interstate individualistic individualistic Bankers Daisy Daisy Mexico materialistic materialistic Chaps Chaps Chaps litde clarinet clarinet clarinet limelight limelight limelight Manson Manson Habsburg Habsburg Strikes Strikes iiber iiber predatory Tax TESTS TESTS TESTS juniors juniors dictatorial dictatorial favored curtailed Handel guaranteeing guaranteeing banana banana selling basal basal McClellan McClellan substantiated MILLER MILLER Graduates Graduates QUESTIONS QUESTIONS Lucinda Kentucky Kentucky fiasco fiasco snapping stalked Norris flicker flicker celebrities gully gully Theodore Mandate strut combinations gloves Franco Franco Franco pictures FATHER FATHER pictured memorized memorized Fisk Harv Harv Harv wei wei solvency solvency damn decadent decadent decadent financiers financiers pact pact pact Issued Issued Dona smacked smacked grinned grinned Mein Mein used optic wedged mobs CCA CCA CCA CCA CCA collectivist collectivist collectivist CCC CCC Graphic Graphic enthusiastically enthusiastically banjo banjo Cincinnati Cincinnati oncoming oncoming oncoming swooped swooped brakes brakes Pittsburgh Pittsburgh Pittsburgh Gutenberg interpretative interpretative negatives negatives bureaus bureaus bureaus Sarkar Sarkar Sarkar Boulder Boulder Recreational Recreational Recreational cotton cotton politicians aprons ingots drag Pickett Pickett graphically graphically ruthlessly ruthlessly shack shack shack Nanking Nanking Nanking Nanking Phosphorus Reorganization Reorganization Reorganization PEACE acreage acreage acreage flickered flickered Laughlin Laughlin picket Kultur Kultur Kultur Cordoba Grid Grid Grid Kirkland America Houghton Houghton commercial noisily noisily Bach Bach Ritz Ritz Ritz Yin Yin anomalies crab Memphis dentists dentists dentists Appropriations Appropriations salad salad averages averages averages bowl jails Thoreau Thoreau Thoreau fostered thunderstorm thunderstorm Bessie Bessie advocated advocated encyclopedia encyclopedia Hongkong Hongkong Hongkong incised incised masses skates seniors seniors Madden littered littered SOUTHERN McCoy McCoy larvae larvae determining stride keel Lanier Lanier Lanier howled Boris Boris Shenandoah Aryan Aryan belligerents belligerents tourists tourists goiter goiter goiter Alumni Alumni coolies coolies bei bei bei Saturdays silage silage silage yawned yawned Connect Connect ultimatum ultimatum serfdom serfdom physique physique subdivisions Credits Credits greased skunk skunk indoor indoor earner earner reorganizing reorganizing dodge dodge Haber Haber tobacco shy hardwood hardwood hardwood capitalists capitalists portent Stonewall Stonewall kicking Aegean Aegean Aegean plankton plankton plankton Music slowly armistice armistice bit asexual asexual Pretty ominously ominously balloon balloon Types sagged sagged sagged stirrups attractively attractively Shinto Shinto Shinto Bloomington Bloomington sputtering sputtering brothels vitamin vitamin vitamin balloons balloons kick Herrmann Herrmann Merely Merely candy candy Larkin Larkin steaming steaming individualist individualist individualist individualism individualism maladjustment maladjustment maladjustment maladjustment Arrange Arrange Waldo Appalachian Appalachian Affecting Affecting education Mining Mining suspiciously grunted grunted phosphorus phosphorus Yearbook Yearbook Yearbook brokers Preferred Preferred trousers trousers cruising vociferous histoire cactus cactus orchestra orchestra orchestra Vitamin Vitamin Vitamin Pub haze foodstuffs foodstuffs foodstuffs astute fantastic nullified nullified guests Piano Piano Farrar Farrar Potassium Potassium Dentistry Dentistry Nordic Nordic Nordic Volcanic gasoline gasoline gasoline gasoline foods foods foods DAVIS DAVIS unforgettable unforgettable unforgettable braids braids undesirable undesirable Juanita Juanita MUSIC MUSIC Civic Civic capitalism capitalism capitalism deck Blum Blum Blum banged banged captions captions weeks sterilization sterilization sterilization unemployed unemployed unemployed Bobbs Bobbs antagonisms antagonisms luck Dressed Expansion Hoare pelagic pelagic pelagic girls girls twisted interlude interlude exposures exposures Purchases Purchases upholstery upholstery upholstery Purchased Nationalists Nationalists Nationalists Alabama Alabama Philippine Philippine definitely definitely definitely Colbert Colbert guilds Havana patted fields Washington Washington feudalism feudalism humanly fluffy fluffy decided conch conch hygiene hygiene hygiene Arctic tiny tiny Physik Physik Physik Georgia Advertiser Athletic Athletic Athletic moratorium moratorium moratorium motorist motorist motorist Ignacio Ignacio realizing realizing butter entanglements entanglements yen yen fullness ducts standardization standardization standardization conciliation tripped Daily Daily unorganized unorganized Including Cameron aphids aphids aphids litmus editor clinics clinics clinics fireworks propaganda propaganda propaganda Agar Agar Rensselaer Rensselaer Wie Wie Wir Wir teeth Orient milk milk surplus surplus Philharmonic Philharmonic Philharmonic finally yeast yeast staples Nellie Nellie Appreciation Appreciation Appreciation Traveling Traveling Changed Variations hummed hummed withdrawals withdrawals withdrawals cranial cranial bob Baldwin crusading crusading undersigned Lansing Lansing Lansing everybody everybody COPYRIGHT COPYRIGHT AMERICA cooperated cooperated Bei Bei Bei ooo ooo curriculum curriculum curriculum Latest Latest Abner Score Coefficient Coefficient Curve tablecloth tablecloth Pliocene Pliocene Mississippi shoes Raid Raid Spengler Spengler Spengler peered peered peasant amperes amperes amperes socialized socialized socialized Kautsky Kautsky Kautsky Kautsky Federated Federated Federated Sigma Sigma Sigma americana americana Yat Yat Yat Yat Yao Yao Probably mining notches Canyon Canyon Canyon orgy orgy orgy CHINA Chaco Chaco Chaco Rockies Rockies Rockies dredging squat squat rehearsals rehearsals pail zigzag Clearing Clearing Test Akademie Akademie plump Kappa Kappa Kappa contracts employers employers ionized ionized ionized secondary secondary Folsom Folsom Education Education Fascist Fascist Fascist Fascist Fascism Fascism Fascism Fascism radiated pharmacists pharmacists tuck orchestration orchestration orchestration decimals ranches ranches ranches Basketball Basketball Basketball Intelligencer Coyote Coyote Coyote Monsignor Monsignor hulls hulls CAPITAL largely mortgages mortgages overcrowding overcrowding flavor flavor Pershing Pershing Pershing Pershing WORDS Napoleonic Napoleonic Napoleonic grid grid grid blankets grin Childs Neo Neo Nez Nez Cardenas Cardenas Cardenas booming booming decomposition seconds manufactured dictators dictators dictators dictators READING READING giggle classifies classifies Ethylene Ethylene Ethylene Bowl Conductor paraphernalia Operators Operators Bartlett hacienda hacienda buggy buggy buggy wiping downstairs downstairs Osage Osage Allgemeine Allgemeine cabled cabled cabled censor resultant resultant Bragg Bragg strikers strikers strikers Terman Terman Terman Terman assets assets resale resale sewer oases oases colleges colleges dollar dollar Willamette Willamette Willamette Station cooperative cooperative cooperative duplicated duplicated Yeast Yeast Fremont Fremont puffy carefree carefree carefree dictatorship dictatorship dictatorship Labels exemptions periodicity periodicity strung ribbon diam diam wrecked plush plush Zool Zool Zool aristocrats aristocrats beacon HCl HCl aptitude barked Pieter Pieter plastered correspondents Tennis FEDERAL FEDERAL FEDERAL staggering Routes phrasing phrasing taut taut purchases Sess Sess Hearst Hearst Hearst Hearst Sophie Sophie Collecting Stevens rifles rifles rounded Texas Texas limped limped inception inception Brisbane Brisbane inordinately biscuits biscuits desirability desirability ironed ironed brewery Northwestern Northwestern Estelle Estelle reparations reparations Propaganda Propaganda Propaganda netted Guerrero Guerrero Ethan Ethan Corbett regulation rates WPA WPA WPA WPA WPA draped Problem Problem trudged trudged Ukrainian Ukrainian Ukrainian Virginia Kit Alpha Alpha Alpha Hawaii Hawaii Hawaii dodging dodging Henderson Suddenly Suddenly Prairie Prairie cynical cynical Knoxville Knoxville burner burner internationalism internationalism internationalism dividends dividends glorification Yen Yen danced gypsy gypsy dances platinum platinum shingles shingles choruses choruses Natur Walters cents cents Gregor Gregor Saratoga permanganate permanganate permanganate shipowners shipowners desks desks appreciable appreciable appreciable PLANT PLANT Copeland Copeland Klux Klux Klux underwriting underwriting underwriting shouting scowled Consulate TYPES TYPES TYPES big big jerked jerked Betty YEAR Harlem semi Dawes Dawes loyalist Hendrick Hendrick Hellenistic Hellenistic Hellenistic Merritt Merritt depressions depressions taxing buccal buccal rumble rumble pumice graduation graduation piled Privately laissez laissez laissez neutrality bankruptcy bored bored frenzied frenzied yearbook yearbook yearbook Versailles scurrying scurrying scurrying spasm aggregates aggregates pineapple pineapple Aztecs Etiology Etiology SEVEN bankers bankers cooked cooked Kellogg Kellogg Kellogg nervousness nervousness sewage sewage uncomfortably Lone junior junior DOCTOR paddle Johnnie Johnnie Johnnie lumber lumber Anna Englanders climb guidance guidance blizzard blizzard blizzard composer composer tap tap mobilisation mobilisation phallus phallus informally informally Ultra Ultra Lambda Lambda Lambda fooling crickets crickets reconstructed curving curving monoxide monoxide monoxide noises Clemenceau Clemenceau Clemenceau Clemenceau Americanism Americanism Americanism Swimming muddle muddle Schriften Balloon pacifism pacifism pacifism China China rel rel postpone franc franc fluctuated Senator Senator retail retail Occasionally Occasionally Reds Reds Reds bodyguard bodyguard compensation Circ Circ curves curves complete Activities Activities Activities Santo mooring sedan callers callers sheathing industrialism industrialism industrialism deutschen deutschen Mechanical dripped dripped scholastic Various Siam phases phases Hawaiian Hawaiian discounts discounts Juana Juana Hawley Stanton Stanton UNIT UNIT UNIT liquidating liquidating busted busted highland basing basing swimmer mouthpiece mouthpiece curtail wurde wurde wurde ridden sausages sausages agar agar agar Cooperatives Cooperatives Cooperatives cubic Grover Grover slits slits spirals spirals Dearborn Dearborn tests tests molding molding molding gulped gulped Fallen crawled crawled scurried scurried scurried whined whined stuffy stuffy stuffy Loyalist Loyalist Loyalist raiding raiding ribbed EDUCATION EDUCATION billions billions billions vegetables Discuss Discuss JANUARY insured insured embargo Imperialism Imperialism Imperialism skating skating rabbits rabbits circus circus Illustrative Illustrative dictatorships dictatorships dictatorships dictatorships Zeitung Zeitung Zeitung Swamp thumping lactose lactose lactose experimenters experimenters parole hats Continental Krupp Krupp Krupp Liabilities Liabilities Liabilities fireplace fireplace school school magnate magnate harried harried guiding kicked Scandinavian Scandinavian electrolysis electrolysis luggage museums museums Andre depreciated grumbled Pasadena Pasadena Pasadena Danzig Danzig Danzig Danzig Sara gadgets gadgets gadgets gadgets Banana Banana bunk bunk bunk badly poker poker kilowatt kilowatt kilowatt Sewage Sewage Sewage Japanese Japanese Japanese jungle jungle jungle Zapata Zapata Zapata Continued Continued tourist Mobile absolutist absolutist Moritz Moritz frantically frantically haired haired Oberlin Oberlin Junior Junior splashed splashed Berliner Berliner jubilant jubilant Traits Traits Carefully Employers Employers Waltz Waltz Haiti Haiti Haiti Fur beiden beiden prefecture prefecture brushed brushed liquefaction acetylene acetylene acetylene steamboat steamboat Ogden Ogden mannerisms mannerisms mannerisms coil coil coil crankshaft crankshaft crankshaft crankshaft colors colors schooners peasants curricular curricular curricular Japan Japan waltz waltz Channing Cristobal Cristobal Quartet Quartet Quartet Magdalena Magdalena licking peroxide peroxide pacifist pacifist pacifist ailments Pei Trotsky Trotsky Trotsky Hays Hays Lumber Lumber Lumber Winifred Winifred compilations smuggled agitators agitators truss truss Publicity Publicity Publicity warehouses Secretary Educators Educators Educators subnormal subnormal subnormal dots Pottery Pottery Latvia Latvia Latvia McConnell McConnell Clothing arousing arousing imperialistic imperialistic imperialistic rhythmically rhythmically niggers niggers Bering Bering Manchuria Manchuria Manchuria Manchuria Nippon Nippon Nippon Nippon meters meters meters Fargo Fargo Fargo securities securities totals totals totals brawl littoral littoral census census Disarmament Disarmament Disarmament Disarmament Eighteenth Eighteenth coiled Pupils Pupils civic civic lavishly RATES RATES fuselage fuselage fuselage fuselage outbreaks outbreaks towing unpretentious unpretentious Tung Tung interrelation interrelation interrelation Mifflin Mifflin clubs clubs Bonnet acclaimed acclaimed propagandists propagandists propagandists erosion erosion erosion liquidate liquidate Dementia Dementia livelihood essentials essentials Dal stove stove Secondary Secondary Athletics Athletics Athletics policeman policeman intangibles intangibles intangibles pork journalistic journalistic Rel Reb Reb Athenaeum Nickel Nickel Nickel Honolulu Honolulu Honolulu tabulation tabulation tabulation Gasoline Gasoline Gasoline INDIVIDUAL INDIVIDUAL graduates graduates delimited delimited newcomer newcomer newcomer Thiers Thiers wrecking wrecking wrecking roared roared conductor conductor DECEMBER flagship flagship dropped hydrogen hydrogen unbelievable unbelievable ballroom ballroom fascist fascist fascist fascist motorists motorists motorists typewritten typewritten typewritten unbelievably unbelievably unbelievably refund refund FOREWORD FOREWORD FOREWORD canary courthouse courthouse Factories nomads appraised appraised sophomore sophomore sophomore oratorio factors factors dorsally dorsally dorsally costal costal hull Felipe explorers explorers gusto gusto swing swing striding striding terrifying appointees appointees wide Tryon diplomatic Hauptmann Hauptmann Hauptmann tanning tanning steers dining freshman freshman freshman librarian librarian NRA NRA NRA NRA NRA Czechs Czechs Czechs Czechs Ellsworth Ellsworth Texan Texan PROBLEM Lyman Lyman swamped IMPORTANT Ass'n Ass'n Ass'n Remembering Remembering joked Shantung Shantung Shantung Shantung Rhineland Rhineland cadets cadets Determining Determining mails mails Buy WAY Pituitary Pituitary Pituitary McLeod McLeod cash cash Colors Colors Jacinto Jacinto Chandler trip trip buys Rockefeller Rockefeller Rockefeller upheaval upheaval ranch ranch unconstitutional Tall standstill standstill CREDIT CREDIT tribesmen tribesmen tribesmen cowboys cowboys cowboys colloidal colloidal colloidal colloidal equalize tariffs tariffs Goebbels Goebbels Goebbels Goebbels labial labial Otto Otto ferric ferric ferric Postmaster Postmaster Basal Basal Basal Bachelor fundamentals fundamentals LEE wholesale wholesale artisans Saito Saito Saito ovoid ovoid accentuated accentuated Mathematics rubber rubber rubber Pulitzer Pulitzer Pulitzer McCormick McCormick McCormick Surplus Surplus Surplus curve curve Textbook Textbook Textbook rimmed rimmed Theoretically Theoretically Theoretically Edison Edison Edison earners earners earners generalize Frequently Frequently activities activities Trimble Trimble Beowulf Beowulf lacquer lacquer lacquer communicable Baylor Baylor Guadalupe warship warship warship Newspapers Newspapers policemen policemen swinging swinging whorl whorl whorl abandonment coerce Paterson airplane airplane airplane airplane swung swung Optic gullies gullies speeding speeding Compensation Compensation pupae pupae Aiken Aiken Belgrade outraged Bureau Bureau Bureau Dental Dental Dental lipped lipped Carnegie Carnegie Carnegie athletics athletics athletics Mort braided etat Aluminum Aluminum Aluminum Oklahoma Oklahoma Oklahoma vocations vocations Selma inaugurated inaugurated aims smoothed cost liabilities liabilities uniforms uniforms skyscrapers skyscrapers skyscrapers mesoderm mesoderm mesoderm middlemen middlemen pianist pianist pianist buff buff galvanometer galvanometer galvanometer pennies Ebert Ebert Ebert Epsilon Epsilon Epsilon inflection unsuited publicity publicity necked Appleton Appleton Oregon Oregon creatine creatine creatine ciliated prelude inertia inertia bandit bandit slots slots slots Metcalf buttermilk buttermilk buttermilk orchestral orchestral orchestral orchestras orchestras orchestras rocking indefinitely Kobe Kobe Kobe ciliary rim rim examiners examiners firearms cartridge cartridge holidays chickens Socialists Socialists Socialists inspirational inspirational Rosario resourcefulness resourcefulness resourcefulness governmental governmental governmental profitably amended sharpened traits traits aptitudes aptitudes aptitudes taxpayers taxpayers constitutionality constitutionality Clubs Clubs dorsum dorsum raided raided raided Jade plaza plaza Bolshevism Bolshevism Bolshevism Bolshevism Elmer Elmer creaked creaked Bryan Bryan settlor settlor settlor potassium potassium apricot Loan Loan capitalized capitalized Huey Huey Huey Huey Hewitt Hewitt peach scrutinized parlor Platte Platte Stearns pottery pottery Longstreet Longstreet Edna Edna Rubinstein Rubinstein Rubinstein Aeschylus Aeschylus Aeschylus buffalo nematodes nematodes nematodes Dodge Dodge peering peering CASES waiters Whitman Whitman melodies melodies plows plows fols fols traveling traveling hopped hopped concessions dass dass dass Lind woefully woefully scrapped scrapped scrapped Lorna Lorna reds reds feverishly feverishly feverishly examinations examinations posteriorly posteriorly volt volt volt iced Curiously Curiously musician cooperate cooperate plebiscite plebiscite plebiscite summers memorize traders barricades Cadet Cadet Vickers Vickers Vickers improvised improvised edging Aztec fingerprint fingerprint fingerprint Ibs Ibs bewilderment bewilderment Deutschen trimmed Weekly Weekly Consumers Consumers Consumers tabloid tabloid tabloid chaos Smuts Smuts Smuts Smuts Mattie Mattie Expense Expense Expense fascism fascism fascism fascism Wadsworth Saar Saar Saar embryology embryology mandibular mandibular mandibular reversed Anton unbearable unbearable pupils pupils olfactory pharmacist pharmacist petrol petrol petrol accountants accountants Everybody Everybody bulging bulging weekly leanings leanings piano piano Chlorine Chlorine dope dope diagnosing diagnosing cubs enlisting proprietorship proprietorship droughts thalamus thalamus stucco symphonies symphonies symphonies Lily tactful tactful tactful Bukharin Bukharin Bukharin appropriations appropriations Ying Ying Ying rooster rooster thud thud trailing trailing elbows issuance issuance student student Houston Houston Croats Croats rocked Downing Salad Salad Recording Newspaper Newspaper Gabe Gabe Gabe Manufacturing Manufacturing Carrie Carrie Foods Foods starred shabby shabby mattresses mattresses weaving Conciliation Conciliation Conciliation Buick Buick Buick cocked travelers travelers squeak continental Bucharest Bucharest Golf Golf Jenny nightgown nightgown rickets rickets rickets cake Vancouver plaques plaques incredibly Poincare Poincare Poincare Poincare'''
#text = 'John has a computer The machine is an PC'
#text = 'poisoner assassin'

words = text.split()

print(words)
print(len(words), len(set(words)))

corpus = [ words[:14], words[14:] ]
print(corpus)

lc = LexicalChainer()
lc.build_disambiguation_graph(words)
lc.wsd()
lc.prune_disambiguation_graph()
lc.trim_disambiguation_graph()
lc.build_chains()
